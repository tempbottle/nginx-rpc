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
    ngx_slab_free_locked(queue->pool, q->elems);
    ngx_slab_free_locked(queue->pool, queue);
    return 0;
}


bool ngx_rpc_queue_task(ngx_rpc_queue_t *queue, void* task)
{
    uint64_t left = queue->capacity - queue->size;

    // left more than process
    if(left <= queue->process_num)
    {
        return false;
    }

    uint64_t pre_write = ngx_atomic_fetch_add(&queue->writeidx , 1);

    task_elem_t * tmp = &(queue->elems[pre_write]);

    void* pre_task = ngx_atomic_swap_set(&tmp->task, task | NGX_WRITE_FLAG);

    if(pre_task == NULL)
    {
        ngx_atomic_fetch_add(&queue->size, 1);
        return true;
    }

    // a proc is wait on this task
    if(pre_task & NGX_READ_FLAG)
    {

        // no other process modify this cell, clear
        while(!ngx_atomic_cmp_set(&tmp->task, task | NGX_WRITE_FLAG, NULL))
        {
            ngx_sched_yield();
        }

        // notify the process a work to do
        queue->notify(pre_task & (~NGX_READ_FLAG), task);
        return true;
    }

    // only one write do the current cell
    //assert((pre_task & 0x1) == false);

    // a task has wait on this cell, restore
    while(! ngx_atomic_cmp_set(&tmp->task, task | NGX_WRITE_FLAG, pre_task))
    {
        //ngx_sched_yield();
    }

    // there has a pre task,restore
    return false;
}


void ngx_rpc_queue_pop(ngx_rpc_queue_t *queue, void** task, void *proc)
{
    for(;;)
    {
        uint64_t pre_read = ngx_atomic_fetch_add(queue->readidx, 1);

        task_elem_t *tmp = &(queue->elems[pre_read]);

        void* pre_task = ngx_atomic_swap_set(&tmp->task, proc | NGX_READ_FLAG);

        // no task,
        if(pre_task == NULL && queue->size == 0)
        {
            return NULL;
        }

        // pre just set
        if(pre_task & NGX_WRITE_FLAG)
        {
            while(!ngx_atomic_cmp_set(tmp->task, proc | NGX_READ_FLAG, NULL))
            {
                ngx_sched_yield();
            }

            ngx_atomic_fetch_add(queue->size, -1);
            return pre_task & (~NGX_WRITE_FLAG);
        }

        //this cell has a process wait move next;
        while(! ngx_atomic_cmp_set(tmp, task | NGX_WRITE_FLAG, pre_task))
        {
            ngx_sched_yield();
        }
    }
}

