#include "ngx_rpc_queue.h"


ngx_rpc_queue_t *ngx_rpc_queue_create(ngx_slab_pool_t *shpool, ngx_log_t* log, int notify_num)
{
    ngx_rpc_queue_t *q = ngx_slab_alloc(shpool, sizeof(ngx_rpc_queue_t));

    if(q == NULL)
    {
        ngx_log_error(NGX_LOG_INFO, log, 0,
                      "ngx_slab_alloc:%p alloc:%d failed ",
                      shpool, sizeof(ngx_rpc_queue_t));
        return NULL;
    }

    q->pool = shpool;
    q->log = log;
    q->notify_num = notify_num;

    if(ngx_shmtx_create(&q->idles_lock, &q->idles_sh, NULL) != NGX_OK)
    {
        ngx_slab_free(shpool, q);
        ngx_log_error(NGX_LOG_INFO, log, 0,
                      "ngx_slab_alloc:%p ngx_shmtx_create failed ",
                      shpool);
        return NULL;
    }

    ngx_queue_init(&q->idles);

    q->notify_slot = ngx_slab_alloc(shpool, sizeof(ngx_rpc_notify_t *) * notify_num);

    int c = 0;

    for( ; c < notify_num; ++c)
    {
        q->notify_slot[c] = ngx_rpc_notify_create(shpool);
    }

    ngx_log_error(NGX_LOG_DEBUG, log, 0,
                  "ngx_rpc_queue_create queue:%p notify_num:%d", q, notify_num);

    return q;
}



int ngx_rpc_queue_destory(ngx_rpc_queue_t *queue)
{
    ngx_log_error(NGX_LOG_INFO, queue->log, 0, "ngx_rpc_queue_destory queue:%p ngx_ncpu", queue);

    int c = 0;
    for( ; c < queue->notify_num; ++c)
    {
        ngx_rpc_notify_free(queue->notify_slot[c]);
    }

    ngx_slab_free(queue->pool, queue->notify_slot);

    ngx_slab_free(queue->pool, queue);
    return 0;
}


int ngx_rpc_queue_push_and_notify(ngx_rpc_queue_t *queue, ngx_rpc_task_t* task)
{

    ngx_queue_t* proc_notify_node = NULL;

    ngx_shmtx_lock(&queue->idles_lock);

    if(!ngx_queue_empty(&queue->idles))
    {
        proc_notify_node = queue->idles.next;

        queue->idles.next = proc_notify_node->next;
        proc_notify_node->next->prev = &queue->idles;
    }

    ngx_shmtx_unlock(&queue->idles_lock);

    if( proc_notify_node == NULL)
    {
        ngx_log_error(NGX_LOG_INFO, queue->log, 0, "ngx_rpc_queue_push_and_notify no proc_notify in idles, queue:%p", queue);

        return NGX_ERROR;
    }

    task->proc_notify = ngx_queue_data(proc_notify_node, ngx_rpc_notify_t, idles);

    ngx_log_error(NGX_LOG_INFO, queue->log, 0,
                  "ngx_rpc_queue_push_and_notify push task:%p to queue:%p with proc_notify eventfd:%d",
                  task, queue, task->proc_notify->event_fd);

    return ngx_rpc_notify_push_task(task->proc_notify , &task->node);

}

ngx_rpc_notify_t * ngx_rpc_queue_get_idle(ngx_rpc_queue_t *queue)
{

    ngx_queue_t* proc_notify_node = NULL;

    ngx_shmtx_lock(&queue->idles_lock);

    if(!ngx_queue_empty(&queue->idles))
    {
        proc_notify_node = queue->idles.next;

        queue->idles.next = proc_notify_node->next;
        proc_notify_node->next->prev = &queue->idles;
    }

    ngx_shmtx_unlock(&queue->idles_lock);

    return ngx_queue_data(proc_notify_node, ngx_rpc_notify_t, idles);
}


ngx_rpc_notify_t *ngx_rpc_queue_add_current_consumer(ngx_rpc_queue_t *queue, void *ctx)
{
    ngx_rpc_notify_t * notify = ngx_rpc_notify_register(queue->notify_slot, ctx);
    return notify;

}

ngx_rpc_notify_t *ngx_rpc_queue_add_current_producer(ngx_rpc_queue_t *queue, void *ctx)
{
    ngx_rpc_notify_t * notify = ngx_rpc_notify_register(queue->notify_slot, ctx);
    return notify;
}








/*
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

*/
