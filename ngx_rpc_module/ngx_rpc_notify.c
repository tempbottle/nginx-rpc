#include "ngx_rpc_notify.h"


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



ngx_rpc_notify_t *ngx_rpc_notify_create(ngx_slab_pool_t *shpool , void *ctx)
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

     if(ngx_shmtx_create(&rpc_ctx->task_lock, &rpc_ctx->shpool, NULL) != NGX_OK)
     {
         ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                       "ngx_palloc error size:%d",
                       sizeof(ngx_http_inspect_ctx_t));

         ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
         return;
     }


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
