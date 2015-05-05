#include <sys/eventfd.h>

#include "ngx_rpc_notify.h"



ngx_rpc_notify_t *ngx_rpc_notify_create(ngx_slab_pool_t *shpool)
{

    ngx_rpc_notify_t *notify = ngx_slab_alloc(shpool, sizeof(ngx_rpc_notify_t));

    notify->shpool = shpool;
    notify->log = ngx_cycle->log;


    if(ngx_shmtx_create(&notify->queue_lock, &notify->queue_sh, NULL) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, notify->log, 0, "ngx_shmtx_create failed");
        ngx_slab_free(shpool, notify);
        return NULL;
    }

    notify->event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

    ngx_queue_init(&notify->queue_head);

    ngx_queue_init(&notify->idles);

    return notify;
}

void ngx_rpc_notify_free(ngx_rpc_notify_t* notify)
{
   // free task

   // free lock
    ngx_slab_free(notify->shpool, notify);
}



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

        notify->read_hanlder(notify->ctx);

    }while(ret > 0);
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



ngx_rpc_notify_t* ngx_rpc_notify_register(ngx_rpc_notify_t **notify_slot, void *ctx)
{

    if( ngx_process_slot < 0 || ngx_process_slot > ngx_last_process)
    {
        ngx_log_error(NGX_LOG_WARN, ngx_cycle->log, 0,
                      "ngx_rpc_notify_register ngx_process_slot:%d not in range [0, %d)",
                      ngx_process_slot, ngx_last_process);
        return NULL;
    }


    ngx_rpc_notify_t* notify = notify_slot[ngx_process_slot];

    notify->ctx = ctx;
    notify->log = ngx_cycle->log;

    notify->notify_conn = ngx_get_connection(notify->event_fd, ngx_cycle->log);

    notify->notify_conn->pool = NULL;
    // sockaddr just keep the current
    notify->notify_conn->sockaddr = (struct sockaddr *)notify;


    // for ngx epoll
    notify->notify_conn->read->handler = ngx_rpc_notify_read_handler;
    notify->notify_conn->read->log     = ngx_cycle->log;
    notify->notify_conn->read->data    = notify->notify_conn;

    notify->notify_conn->write->handler = ngx_rpc_notify_write_handler;
    notify->notify_conn->write->log     = ngx_cycle->log;
    notify->notify_conn->write->data    = notify->notify_conn;

    notify->read_hanlder = ngx_rpc_notify_default_hanlder;
    notify->write_hanlder = ngx_rpc_notify_default_hanlder;


    if( ngx_add_conn(notify->notify_conn) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_WARN, ngx_cycle->log, 0,
                      "ngx_rpc_notify_register failed, notify:%p eventfd:%d slot:%d",
                      notify, notify->event_fd, ngx_process_slot);
        return NULL;
    }

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                  "ngx_rpc_notify_register notify:%p eventfd:%d slot:%d",
                  notify, notify->event_fd, ngx_process_slot);

    return notify;
}


int ngx_rpc_notify_unregister(ngx_rpc_notify_t* notify)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ngx_cycle->log, 0,
                  "ngx_rpc_notify_unregister notify:%p eventfd:%d",
                  notify, notify->event_fd);

    ngx_del_conn(notify->notify_conn, 0);
    close(notify->event_fd);

    notify->notify_conn->pool = NULL;
    ngx_free_connection(notify->notify_conn);

    return NGX_OK;
}



int ngx_rpc_notify_push_task(ngx_rpc_notify_t* notify, ngx_queue_t* node)
{
    ngx_shmtx_lock(&notify->queue_lock);

    //push tail
    node->prev = notify->queue_head.prev;
    notify->queue_head.prev = node;

    node->prev->next = node;
    node->next = &notify->queue_head;

    ngx_shmtx_unlock(&notify->queue_lock);


    eventfd_write((notify)->event_fd, 1);

    ngx_log_error(NGX_LOG_INFO, notify->log, 0, "ngx_rpc_notify_push_task notify:%p eventfd:%d node:%p ", notify, notify->event_fd, node);
    return NGX_OK;
}

int ngx_rpc_notify_pop_task(ngx_rpc_notify_t* notify, ngx_queue_t**node)
{
    ngx_shmtx_lock(&notify->queue_lock);

    if(ngx_queue_empty(&notify->queue_head))
    {
        *node = NULL;
    }else{
        *node = notify->queue_head.next;

        // remove head
        notify->queue_head.next = (*node)->next;
        notify->queue_head.next->prev= &notify->queue_head;
    }

    ngx_shmtx_unlock(&notify->queue_lock);

    ngx_log_error(NGX_LOG_INFO, notify->log, 0, "ngx_rpc_notify_pop_task notify:%p eventfd:%d node:%p ", notify, notify->event_fd, *node);

    return *node == NULL ? NGX_ERROR :NGX_OK;
}

