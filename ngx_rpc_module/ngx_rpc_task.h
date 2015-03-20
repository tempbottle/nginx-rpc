
#ifndef _NGX_RPC_TASK_H_
#define _NGX_RPC_TASK_H_

#include <ngx_core.h>

struct ngx_rpc_task_t;

// next = filter(ctx, pre, task)
typedef void* (*ngx_task_filter)(void*ctx, ngx_rpc_task_t* pre, ngx_rpc_task_t* task);


// http key value
typedef struct {
    ngx_queue_t next;
    const char *key;
    void* value;
} ngx_rpc_task_params_t;


// for which process the task process
typedef enum {
   PROCESS_IN_REQUEST    = 0,
   PROCESS_IN_PROC       = 1,
   PROCESS_IN_SUBREQUEST = 2,
} task_type_t;


typedef struct {
    //task stack
    ngx_queue_t next;

    // for sub request
    const char *path;
    ngx_queue_t params;

    // for rpc request req & res
    ngx_chain_t req_bufs;
    ngx_chain_t res_bufs;

    ngx_task_filter filter;
    task_type_t type;

    // mertics
    int response_states;
    int init_time_ms;
    int time_out_ms;
    int done_ms;

    ngx_slab_pool_t *pool;

} ngx_rpc_task_t;


///
/// \brief ngx_http_rpc_destry_task
/// \param t
///

ngx_rpc_task_t * ngx_http_rpc_task_create(ngx_slab_pool_t *pool, void *ctx);

void ngx_http_rpc_task_destory(ngx_rpc_task_t *t);

#endif
