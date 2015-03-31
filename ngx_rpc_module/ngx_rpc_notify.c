#include <sys/eventfd.h>

#include "ngx_rpc_notify.h"


static void ngx_rpc_notify_default_hanlder(void* dummy){}

static void ngx_rpc_notify_read_handler(ngx_event_t *ev){
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0,"ngx_event_hanlder_notify_read");

    //read and do all the work;
    ngx_int_t signal = 1;
    ngx_rpc_notify_t * cn = ev->data;

    int ret = 0;
    do{
        ret = read(cn->event_fd, (char*)&signal, sizeof(signal));
    }while(ret > 0);

    ngx_rpc_notify_t *n = (ngx_rpc_notify_t *)ev->data;
    n->read_hanlder(n->ctx);

}

static void ngx_rpc_notify_write_handler(ngx_event_t *ev)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0, "ngx_event_hanlder_notify_write");
    ngx_del_event(ev, NGX_WRITE_EVENT, 0);

    ngx_rpc_notify_t *n = (ngx_rpc_notify_t *)ev->data;
    n->write_hanlder(n->ctx);
}



ngx_rpc_notify_t *ngx_rpc_notify_create(ngx_slab_pool_t *shpool , void *ctx)
{

    ngx_rpc_notify_t *notify = ngx_slab_alloc_locked(shpool, sizeof(ngx_rpc_notify_t));

     notify->ctx = ctx;
     notify->event_fd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
     notify->notify_conn = ngx_get_connection(notify->event_fd, ngx_cycle->log);


     notify->notify_conn->pool = NULL;
     notify->notify_conn->read->handler = ngx_rpc_notify_read_handler;
     notify->notify_conn->read->log = ngx_cycle->log;
     notify->notify_conn->read->data = notify;

     notify->notify_conn->write->handler = ngx_rpc_notify_write_handler;
     notify->notify_conn->write->log = ngx_cycle->log;
     notify->notify_conn->write->data = notify;

     notify->read_hanlder = ngx_rpc_notify_default_hanlder;
     notify->write_hanlder = ngx_rpc_notify_default_hanlder;

     if(ngx_shmtx_create(&notify->lock_task, &notify->psh, NULL) != NGX_OK)
     {
         return NULL;
     }

     if( ngx_add_conn(notify->notify_conn) != NGX_OK)
     {
         return NULL;
     }

     ngx_shmtx_lock(&notify->lock_task);
     ngx_queue_init(&notify->task);
     ngx_shmtx_unlock(&notify->lock_task);

     return notify;
}

int ngx_rpc_notify_destory(ngx_rpc_notify_t* notify)
{
    ngx_del_conn(notify->notify_conn, 0);
    close(notify->event_fd);
    notify->notify_conn->pool = NULL;
    ngx_free_connection(notify->notify_conn);
    return NGX_OK;
}

int ngx_rpc_notify_task(ngx_rpc_notify_t* notify, void(*hanlder)(void*), void* data)
{

    ngx_rpc_notify_task_t * t = ngx_slab_alloc_locked(notify->shpool, sizeof(ngx_rpc_notify_task_t));

    t->hanlder = hanlder;
    t->ctx = data;

    ngx_shmtx_lock(&notify->lock_task);
    ngx_queue_add(&notify->task, &t->next);
    ngx_shmtx_unlock(&notify->lock_task);

    ngx_int_t signal = 1;
    write(notify->event_fd, (char*)&signal, sizeof(signal));

    return NGX_OK;
}
