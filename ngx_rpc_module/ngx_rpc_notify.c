#include <sys/eventfd.h>

#include "ngx_rpc_notify.h"


static void ngx_rpc_notify_default_hanlder(void* dummy){}

static void ngx_rpc_notify_read_handler(ngx_event_t *ev){


    ngx_connection_t *notify_con = ev->data;
    ngx_rpc_notify_t *notify = (ngx_rpc_notify_t *)(notify_con->sockaddr);



    //read and do all the work;
    ngx_uint_t signal = 1;

    int ret = 0;
    do{
        ret = read(notify->event_fd, (char*)&signal, sizeof(signal));

        ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0,
                      "ngx_rpc_notify_read_handler notify:%p eventfd:%d signal:%d ,ret:%d",
                      notify, notify->event_fd, signal, ret);
    }while(ret > 0);

    notify->read_hanlder(notify->ctx);

}

static void ngx_rpc_notify_write_handler(ngx_event_t *ev)
{

    ngx_connection_t *notify_con = ev->data;
    ngx_rpc_notify_t *notify = (ngx_rpc_notify_t *)(notify_con->sockaddr);


    ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0,
                  "ngx_rpc_notify_write_handler notify:%p eventfd:%d",
                  notify, notify->event_fd);

    ngx_del_event(notify_con->write, NGX_WRITE_EVENT, 0);

    notify->write_hanlder(notify->ctx);
}



ngx_rpc_notify_t *ngx_rpc_notify_create(ngx_slab_pool_t *shpool , void *ctx)
{

    ngx_rpc_notify_t *notify = ngx_slab_alloc(shpool, sizeof(ngx_rpc_notify_t));

     notify->ctx = ctx;
     notify->log = ngx_cycle->log;
     notify->event_fd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
     notify->notify_conn = ngx_get_connection(notify->event_fd, ngx_cycle->log);


     notify->notify_conn->pool = NULL;
     notify->notify_conn->sockaddr = (struct sockaddr *)notify;


     notify->notify_conn->read->handler = ngx_rpc_notify_read_handler;
     notify->notify_conn->read->log     = ngx_cycle->log;
     notify->notify_conn->read->data    = notify->notify_conn;

     notify->notify_conn->write->handler = ngx_rpc_notify_write_handler;
     notify->notify_conn->write->log     = ngx_cycle->log;
     notify->notify_conn->write->data    = notify->notify_conn;

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

     ngx_log_debug(NGX_LOG_DEBUG_ALL, notify->log, 0,
                   "ngx_rpc_notify_create notify:%p eventfd:%d",
                   notify, notify->event_fd);

     return notify;
}

int ngx_rpc_notify_destory(ngx_rpc_notify_t* notify)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, notify->log, 0,
                  "ngx_rpc_notify_destory notify:%p eventfd:%d",
                  notify, notify->event_fd);

    ngx_del_conn(notify->notify_conn, 0);
    close(notify->event_fd);

    notify->notify_conn->pool = NULL;
    ngx_free_connection(notify->notify_conn);
    return NGX_OK;
}

int ngx_rpc_notify_task(ngx_rpc_notify_t* notify, void(*hanlder)(void*), void* data)
{

    ngx_rpc_notify_task_t * t = ngx_slab_alloc(notify->shpool, sizeof(ngx_rpc_notify_task_t));

    t->hanlder = hanlder;
    t->ctx = data;
    ngx_queue_init(&t->next);

    ngx_shmtx_lock(&notify->lock_task);
    ngx_queue_add(&notify->task, &t->next);
    ngx_shmtx_unlock(&notify->lock_task);

    ngx_uint_t signal = 1;
    write(notify->event_fd, (char*)&signal, sizeof(signal));

    ngx_log_debug(NGX_LOG_DEBUG_ALL, notify->log, 0,
                  "ngx_rpc_notify_task notify:%p eventfd:%d data:%p",
                  notify, notify->event_fd, data);
    return NGX_OK;
}
