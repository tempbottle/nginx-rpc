#include "ngx_rpc_queue.h"

ngx_rpc_queue_t *ngx_rpc_queue_create(ngx_slab_pool_t *shpool, int max_elem)
{
    ngx_rpc_queue_t *q = ngx_slab_alloc_locked(shpool, sizeof(ngx_rpc_queue_t));

    if(q == NULL)
    {
        return NULL;
    }

    q->capacity = max_elem;
    q->process_num = ngx_ncpu;
    q->pool = shpool;

    q->size = q->readidx = q->writeidx = 0;
    q->elems = ngx_slab_alloc_locked(shpool, q->capacity * sizeof(task_elem_t));

    if(q->elems == NULL)
    {
        return NULL;
    }

    memset(q->elems, 0, q->capacity *sizeof(task_elem_t));

    return q;
}

int ngx_rpc_queue_destory(ngx_rpc_queue_t *queue)
{
    ngx_slab_free_locked(queue->pool, queue->elems);
    ngx_slab_free_locked(queue->pool, queue);
    return 0;
}


int ngx_rpc_queue_push(ngx_rpc_queue_t *queue, void* task)
{
    uint64_t left = queue->capacity - queue->size;

    // left more than process
    if(left <= queue->process_num)
    {
        return 0;
    }

    uint64_t pre_write = ngx_atomic_fetch_add(&queue->writeidx , 1);

    task_elem_t * tmp = &(queue->elems[pre_write]);

    void* pre_task = ngx_atomic_swap_set((void**)&tmp->task, (void*)((int64_t)task | NGX_WRITE_FLAG));

    if(pre_task == NULL)
    {
        ngx_atomic_fetch_add(&queue->size, 1);
        return 1;
    }

    // a proc is wait on this task
    if( ((int64_t)pre_task) & NGX_READ_FLAG)
    {

        // no other process modify this cell, clear
        while(!ngx_atomic_cmp_set(&tmp->task, ((int64_t)task | NGX_WRITE_FLAG), NULL))
        {
            ngx_sched_yield();
        }

        // notify the process a work to do
        queue->notify((void*)((int64_t)pre_task & (~NGX_READ_FLAG)), task);
        return 1;
    }

    // only one write do the current cell
    //assert((pre_task & 0x1) == false);

    // a task has wait on this cell, restore
    while(! ngx_atomic_cmp_set(&tmp->task, (void*)((int64_t)task | NGX_WRITE_FLAG), pre_task))
    {
        //ngx_sched_yield();
    }

    // there has a pre task,restore
    return 0;
}


void* ngx_rpc_queue_pop(ngx_rpc_queue_t *queue, void *proc)
{
    for(;;)
    {
        uint64_t pre_read = ngx_atomic_fetch_add(&(queue->readidx), 1) ;

        task_elem_t *tmp = &(queue->elems[pre_read]);

        void* pre_task = ngx_atomic_swap_set((void**)&tmp->task, (void*)((int64_t)proc | NGX_READ_FLAG));

        // no task,
        if(pre_task == NULL && queue->size == 0)
        {
            return NULL;
        }

        // pre just set
        if( (int64_t)(pre_task) & NGX_WRITE_FLAG)
        {
            while( !ngx_atomic_cmp_set(&tmp->task, (void*)((int64_t)(proc) | NGX_READ_FLAG), NULL) )
            {
                ngx_sched_yield();
            }

            ngx_atomic_fetch_add(&queue->size, -1);
            return (void*)((int64_t)(pre_task) & (~NGX_WRITE_FLAG));
        }

        //this cell has a process wait move next;
        while(! ngx_atomic_cmp_set((void**)(queue->elems + pre_read), (void*)((int64_t)tmp->task | NGX_WRITE_FLAG), pre_task))
        {
            ngx_sched_yield();
        }
    }
}

