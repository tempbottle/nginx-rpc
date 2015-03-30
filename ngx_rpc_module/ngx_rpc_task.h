#ifndef _NGX_RPC_TASK_H_
#define _NGX_RPC_TASK_H_


typedef unsigned short u_short;

#include <ngx_core.h>




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

typedef enum {
   TASK_INIT          = 0,
   TASK_PROCING       = 1,
   TASK_DONE          = 2,
} task_status_t;


#define MAX_PRE_TASK_NUM 4

struct __ngx_rpc_task_t {
    ngx_slab_pool_t *pool;
    ngx_queue_t pending;
    ngx_queue_t done;

    // for sub request
    char *path;
    ngx_queue_t params;

    // for rpc request req & res
    ngx_chain_t req_bufs;
    ngx_chain_t res_bufs;
    ngx_uint_t  res_length;

    // clousre
    void (*filter)(void* ctx, struct __ngx_rpc_task_t* task);
    void* ctx;


    // mertics
    int response_states;
    int init_time_ms;
    int time_out_ms;
    int done_ms;

    // for destory
    volatile ngx_uint_t refcount;
    task_type_t type:4;
    task_status_t status:4;

};

typedef struct __ngx_rpc_task_t ngx_rpc_task_t;


///
/// \brief ngx_http_rpc_destry_task
/// \param t
///

ngx_rpc_task_t * ngx_http_rpc_task_create(ngx_slab_pool_t *pool, void *ctx);

void ngx_http_rpc_task_destory(ngx_rpc_task_t *t);

#define ngx_http_rpc_task_ref_add(t) \
    ngx_atomic_fetch_add(&(t->refcount), 1)

#define ngx_http_rpc_task_ref_sub(t) \
    if(0 == ngx_atomic_fetch_add(&(t->refcount), -1)) \
         ngx_http_rpc_task_destory(t);

extern  ngx_rbtree_node_t sentinel;

#endif
