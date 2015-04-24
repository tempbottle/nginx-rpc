#ifndef _NGX_RPC_NOTIFY_H_
#define _NGX_RPC_NOTIFY_H_



#include <assert.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>
#include <sys/eventfd.h>


#define ngx_atomic_swap_set(x,y) __sync_lock_test_and_set(x, y)


/// a notify object
typedef struct {

    // pending task
    ngx_shmtx_sh_t queue_sh;
    ngx_shmtx_t    queue_lock;
    ngx_queue_t    queue_head;

    // idles
    ngx_queue_t    idles;

    ngx_log_t        *log;
    ngx_slab_pool_t  *shpool;
    ngx_connection_t *notify_conn;
    int event_fd;


    void(*read_hanlder)(void* );
    void(*write_hanlder)(void* );
    void *ctx;
} ngx_rpc_notify_t;


// create
ngx_rpc_notify_t* ngx_rpc_notify_create(ngx_slab_pool_t *shpool );
void ngx_rpc_notify_free(ngx_rpc_notify_t* notify);


// init in process
ngx_rpc_notify_t* ngx_rpc_notify_register(ngx_rpc_notify_t **notify_slot , void*ctx);
int ngx_rpc_notify_unregister(ngx_rpc_notify_t* notify);


int ngx_rpc_notify_push_task(ngx_rpc_notify_t* notify, ngx_queue_t* node);

int ngx_rpc_notify_pop_task(ngx_rpc_notify_t* notify, ngx_queue_t**node);




///////





#endif




