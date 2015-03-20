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

    ngx_slab_pool_t *shpool;

    ngx_connection_t* notify_conn;
    int eventf_fd;

    // pending task
    ngx_queue_t task;
    ngx_shmtx_t lock_task;

    void(*read_hanlder)(void* );
    void(*write_hanlder)(void* );
    void *ctx;

} ngx_rpc_notify_t;


// handler for event fd
static void ngx_rpc_nofiy_default_hanlder(void*);

static int  ngx_rpc_nofiy_read_handler(ngx_event_t *ev);

static void ngx_event_hanlder_notify_write(ngx_event_t *ev);


ngx_rpc_notify_t* ngx_rpc_notify_create(ngx_slab_pool_t *shpool , void *ctx);
int ngx_rpc_notify_destory(ngx_rpc_notify_t* notify);

int ngx_rpc_notify_task(ngx_rpc_notify_t* notify, void(*hanlder)(void*), void* data);

///////





#endif




