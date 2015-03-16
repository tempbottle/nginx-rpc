
#include "ngx_rpc_queue.h"

int ngx_rpc_queue_create(ngx_rpc_queue_t **queue, ngx_cycle_t * cycle)
{

}

int ngx_rpc_queue_destory(ngx_rpc_queue_t *queue)
{


}


bool ngx_rpc_push_task(ngx_rpc_queue_t *queue, void* task, void *proc)
{
    uint64_t left = queue->capacity - queue->size;

    // left more than process
    if(left <= queue->process_num)
    {
        return false;
    }

    uint64_t pre_write = ngx_atomic_fetch_add(queue->writeidx , 1);

    task_elem_t * tmp = &(queue->elems[pre_write]);

    void* pre_task = ngx_atomic_swap_set(tmp->task, task | NGX_WRITE_FLAG);

    if(pre_task == NULL)
    {
        ngx_atomic_fetch_add(queue->size, NULL);
        return true;
    }

    if(pre_task & NGX_READ_FLAG)
    {

        while(!ngx_atomic_cmp_set(tmp->task, task | NGX_WRITE_FLAG, NULL))
        {
            ngx_sched_yield();
        }

        queue->process(pre_task & (~NGX_READ_FLAG), task);
        return true;
    }

    // only one write do the current cell
    //assert((pre_task & 0x1) == false);

    while(! ngx_atomic_cmp_set(tmp, task | NGX_WRITE_FLAG, pre_task))
    {
        //ngx_sched_yield();
    }

    // there has a pre task,restore
    return false;
}


void  ngx_rpc_pop_task_block(ngx_rpc_queue_t *queue, void** task, void *proc)
{
    for(;;)
    {
        uint64_t pre_read = ngx_atomic_fetch_add(queue->readidx, 1);

        task_elem_t *tmp = &(queue->elems[pre_read]);

        void* pre_task = ngx_atomic_swap_set(tmp->task, proc | NGX_READ_FLAG);

        // no task
        if(pre_task == NULL)
        {
            queue->wait(proc, task);
            return;
        }

        // pre just set
        if(pre_task & NGX_WRITE_FLAG)
        {
            while(!ngx_atomic_cmp_set(tmp->task, proc | NGX_READ_FLAG, NULL))
            {
                ngx_sched_yield();
            }

            ngx_atomic_fetch_add(queue->size, -1);
            queue->process(proc,  pre_task & (~NGX_WRITE_FLAG));
            return;
        }


        //this cell task not process done , resotre
        while(! ngx_atomic_cmp_set(tmp, task | NGX_WRITE_FLAG, pre_task))
        {
            ngx_sched_yield();
        }

    }
}




void ngx_rpc_nofiy_default_hanlder(void*){}

int ngx_rpc_nofiy_read_handler(ngx_event_t *ev){
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0,"ngx_event_hanlder_notify_read");

    //read and do all the work;
    ngx_int_t signal = 1;

    int ret = 0;
    do{
        ret = ::read(notify_conn->fd, (char*)&signal, sizeof(signal));
    }while(ret > 0);

    ngx_rpc_notify_t *n = (ngx_rpc_notify_t *)ev->data;
    n->read_hanlder(n->ctx);

    return 0;
}

static void ngx_event_hanlder_notify_write(ngx_event_t *ev)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0, "ngx_event_hanlder_notify_write");
    ngx_del_event(ev, NGX_WRITE_EVENT, 0);

    ngx_rpc_notify_t *n = (ngx_rpc_notify_t *)ev->data;
    n->write_hanlder(n->ctx);
}



int ngx_rpc_notify_create(ngx_rpc_notify_t* notify, ngx_cycle_t *cycle, ngx_slab_pool_t *shpool , void *ctx)
{


     notify->ctx = ctx;
     notify->event_fd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
     notify->notify_conn = ngx_get_connection(notify->eventf_fd, cycle->log);


     notify->notify_conn->pool = cycle->pool;
     notify->notify_conn->read->handler = ngx_rpc_nofiy_read_handler;
     notify->notify_conn->read->log = cycle->log;
     notify->notify_conn->read->data = notify;

     notify->notify_conn->write->handler = ngx_event_hanlder_notify_write;
     notify->notify_conn->write->log = cycle->log;
     notify->notify_conn->write->data = notify;

     notify->read_hanlder = ngx_rpc_nofiy_default_hanlder;
     notify->write_hanlder = ngx_rpc_nofiy_default_hanlder;

     if( ngx_add_conn(notify->notify_conn) != NGX_OK)
     {
         return NGX_ERROR;
     }

     ngx_shmtx_lock(&notify->lock_task);
     ngx_queue_init(&notify->task);
     ngx_shmtx_unlock(&notify->lock_task);

     return NGX_OK;
}

int ngx_rpc_notify_destory(ngx_rpc_notify_t* notify)
{
    ngx_del_conn(notify->notify_conn);
    ::close(notify->eventf_fd);
    notify->notify_conn->pool = NULL;
    ngx_free_connection(notify->notify_conn);

}

int ngx_rpc_notify_task(ngx_rpc_notify_t* notify, void(*hanlder)(void*), void* data)
{

    ngx_rpc_notify_task_t *  t = ngx_slab_alloc_locked(notify->shpool, sizeof(ngx_rpc_notify_task_t));
    t->hanlder = hanlder;
    t->ctx = data;

    ngx_shmtx_lock(&notify->lock_task);
    ngx_queue_add(&notify->task, &t->next);
    ngx_shmtx_unlock(&notify->lock_task);

    ngx_int_t signal = 1;
    ::write(notify->eventf_fd, (char*)&signal, sizeof(signal));

    return NGX_OK;
}
