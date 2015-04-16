#include "ngx_rpc_queue.h"

ngx_rpc_queue_t *ngx_rpc_queue_create(ngx_slab_pool_t *shpool)
{
    ngx_rpc_queue_t *q = ngx_slab_alloc(shpool, sizeof(ngx_rpc_queue_t));

    if(q == NULL)
    {
         return NULL;
    }

    q->pool = shpool;
    q->log = ngx_cycle->log;

    if(ngx_shmtx_create(&q->procs_lock, &q->procs_sh, NULL) != NGX_OK)
    {
        ngx_slab_free(shpool, q);
        return NULL;
    }

    ngx_queue_init(idles);

    q->producer = ngx_rpc_notify_create(shpool);
    q->consumer = ngx_rpc_notify_create(shpool);

    gx_log_error(NGX_LOG_DEBUG, ngx_cycle->log, 0,
                 "ngx_rpc_queue_create queue:%p producer:%d consumer:%d",
                 q, q->producer->event_fd , q->consumer->event_fd);

    return q;
}


ngx_rpc_notify_t *ngx_rpc_queue_add_current_producer(ngx_rpc_queue_t *queue)
{
     ngx_rpc_notify_init(p->notify, p);
     return queue->producer;
}

ngx_rpc_notify_t *ngx_rpc_queue_add_current_consumer(ngx_rpc_queue_t *queue)
{
    ngx_rpc_notify_init(p->notify, p);
    return  queue->consumer;
}



int ngx_rpc_queue_destory(ngx_rpc_queue_t *queue)
{
    ngx_log_error(NGX_LOG_INFO,queue->log, 0, "ngx_rpc_queue_destory queue:%p ngx_ncpu", queue);

    ngx_rpc_notify_destory(queue->producer);
    ngx_rpc_notify_destory(queue->consumer);

    ngx_slab_free(queue->pool, queue);
    return 0;
}


int ngx_rpc_queue_push(ngx_rpc_queue_t *queue, void* task)
{


    uint64_t left = queue->capacity - queue->size;
    ngx_log_error(NGX_LOG_INFO,queue->log, 0, "ngx_rpc_queue_push queue:%p task:%p, left:%d, capacity:%d, size:%d process_num:%d", \
                  queue, task, left, queue->capacity, queue->size , queue->proc_num);

    // left more than process
    if(left <= queue->proc_num)
    {
        return 0;
    }

    uint64_t pre_write = ngx_atomic_fetch_add(&queue->writeidx , 1) % queue->capacity;

    task_elem_t * tmp = (queue->elems + pre_write);

    void* pre_task = ngx_atomic_swap_set((void**)&tmp->task, (void*)((int64_t)task | NGX_TASK_FLAG));



    if(pre_task == NULL)
    {
        ngx_atomic_fetch_add(&queue->size, 1);
        ngx_log_error(NGX_LOG_INFO,queue->log, 0, "ngx_rpc_queue_push only pre_write:%d pre_task:%p, tmp:%p size:%d ", \
                      pre_write, pre_task, tmp->task, queue->size);
        return 1;
    }

    // a proc is wait on this task
    if( ((int64_t)pre_task) & NGX_NOTIFY_FLAG)
    {

        // no other process modify this cell, clear
        while(!ngx_atomic_cmp_set(&tmp->task, ((int64_t)task | NGX_TASK_FLAG), NULL))
        {
            ngx_sched_yield();
        }

        // notify the process a work to do
        ngx_rpc_notify_t * n = (ngx_rpc_notify_t *)((int64_t)pre_task & (~NGX_NOTIFY_FLAG));
        ngx_rpc_notify_task(n, NULL, task);

        //ngx_atomic_fetch_add(&queue->size, 1);

        ngx_log_error(NGX_LOG_INFO,queue->log, 0, "ngx_rpc_queue_push and notify:%p  pre_write:%d pre_task:%p, tmp:%p size:%d ",
                      n, pre_write, pre_task, tmp->task, queue->size);
        return 1;
    }

    // only one write do the current cell
    //assert((pre_task & 0x1) == false);

    // a task has wait on this cell, restore
    while(! ngx_atomic_cmp_set(&tmp->task, (void*)((int64_t)task | NGX_TASK_FLAG), pre_task))
    {
        ngx_log_error(NGX_LOG_INFO,queue->log, 0,
                      "ngx_rpc_queue_push  slot busy pre_write:%d pre_task:%p, tmp:%p size:%d ",
                      pre_write, pre_task, tmp->task, queue->size);
        ngx_sched_yield();
    }

    // there has a pre task,restore
    return 0;
}


void* ngx_rpc_queue_pop(ngx_rpc_queue_t *queue, ngx_rpc_notify_t *notify)
{
    for(;;)
    {
        uint64_t pre_read = ngx_atomic_fetch_add(&(queue->readidx), 1) % queue->capacity;

        task_elem_t *tmp = (queue->elems + pre_read);

        void* pre_task = ngx_atomic_swap_set((void**)&tmp->task, (void*)((int64_t)notify | NGX_NOTIFY_FLAG));

        // no task,
        if(pre_task == NULL && queue->size == 0)
        {
            ngx_log_error(NGX_LOG_INFO,queue->log, 0,
                          "ngx_rpc_queue_pop wait  pre_read:%d pre_task:%p, notify:%p tmp:%p size:%d ", \
                          pre_read, pre_task, notify, tmp->task, queue->size);
            return NULL;
        }

        // pre just set
        if((int64_t)(pre_task) & NGX_TASK_FLAG)
        {
            while( !ngx_atomic_cmp_set(&tmp->task, (void*)((int64_t)(notify) | NGX_NOTIFY_FLAG), NULL) )
            {
                ngx_sched_yield();
            }

            ngx_atomic_fetch_add(&queue->size, -1);

            ngx_log_error(NGX_LOG_INFO,queue->log, 0, "ngx_rpc_queue_pop task \
                          pre_read:%d pre_task:%p, notify:%p tmp:%p size:%d ", \
                          pre_read, pre_task, notify, tmp->task, queue->size);

            return (void*)((int64_t)(pre_task) & (~NGX_TASK_FLAG));
        }

        //this cell has a process wait move next;
        while(! ngx_atomic_cmp_set(&tmp->task, (void*)((int64_t)tmp->task | NGX_NOTIFY_FLAG), pre_task))
        {
            ngx_sched_yield();

            ngx_log_error(NGX_LOG_INFO,queue->log, 0,
                          "ngx_rpc_queue_pop busy pre_read:%d pre_task:%p, proc:%p tmp:%p size:%d ",
                          pre_read, pre_task, notify, tmp->task, queue->size);
        }
    }
}

