#ifndef _NGX_RPC_NOTIFY_H_
#define _NGX_RPC_NOTIFY_H_



#include <assert.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>


#define ngx_atomic_swap_set(x,y) __sync_lock_test_and_set(x, y)


/// a task pending on  notify
typedef struct {
     void(*hanlder)(void*);
     void* ctx;
     ngx_queue_t next;
} ngx_rpc_notify_task_t;


/// a notify object
typedef struct {

    ngx_log_t *log;
    ngx_slab_pool_t *shpool;
    ngx_connection_t *notify_conn;
    int event_fd;

    // notify call back
    void(*read_hanlder)(void* );
    void(*write_hanlder)(void* );
    void *ctx;
} ngx_rpc_notify_t;


// handler for event fd


ngx_rpc_notify_t* ngx_rpc_notify_create(ngx_slab_pool_t *shpool );
ngx_rpc_notify_t* ngx_rpc_notify_init(ngx_rpc_notify_t *notify , void*ctx);
int ngx_rpc_notify_destory(ngx_rpc_notify_t* notify);


#define ngx_rpc_notify_trigger(notify) \
    eventfd_write(notify->event_fd, 1)


///////





#endif




