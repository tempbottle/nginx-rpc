#ifndef _NGX_RPC_TASK_H_
#define _NGX_RPC_TASK_H_

#include <ngx_core.h>
#include "ngx_rpc_notify.h"

#define MAX_PATH_NAME 128

// http key value
typedef struct {
    ngx_queue_t next;
    const char *key;
    void* value;
} ngx_rpc_task_params_t;

typedef struct ngx_rpc_task_s ngx_rpc_task_t;


typedef struct {
    void (*handler)(ngx_rpc_task_t* _this, void *p1);
    void* p1;
} task_closure_t;

#define task_closure_exec(t) t->closure.handler(t, t->closure.p1)

///
typedef struct {
    int line;
    const char* fun;
    const char* file;
    ngx_uint_t create_ms;
    ngx_uint_t done_ms;
} task_metric_t ;



///
struct ngx_rpc_task_s {

    ngx_queue_t node;
    ngx_rpc_notify_t *proc_notify;
    ngx_rpc_notify_t *done_notify;
    volatile ngx_uint_t refcount;


    // closure
    task_closure_t closure;
    task_closure_t finish;
    ngx_slab_pool_t *pool;
    ngx_log_t * log;

    // for sub request
    u_char path[MAX_PATH_NAME];
    ngx_queue_t params;

    // for rpc request req & res
    ngx_chain_t req_bufs;
    ngx_int_t  req_length;
    ngx_chain_t res_bufs;
    ngx_int_t  res_length;
    // mertics
    int response_states;
    ngx_uint_t time_out_ms;

    task_metric_t metric;


};




//// Some helper
#define task_closure_init(t,h,p1) \
    t->closure.handler = h ; \
    t->closure.p1 = p1;

#define task_metric_init(t) \
    t->metric.line = __LINE__; \
    t->metric.fun  = __FUNC__; \
    t->metric.file  = __FILE__; \
    t->metric.create_ms = ngx_current_msec;


#define ngx_http_rpc_task_ref_add(t) \
    ngx_atomic_fetch_add(&(t->refcount), 1)

#define ngx_http_rpc_task_ref_sub(t) \
    if(0 == ngx_atomic_fetch_add(&(t->refcount), -1)) \
    ngx_http_rpc_task_destory(t);

#define ngx_http_rpc_task_trace(t) \
    ngx_log_error(NGX_LOG_DEBUG, t->log, 0, \
        "interface:%s, handler:%p p1:%p  init:%u, done:%u from:%s:%s%d", \
        t->interface, \
        t->closure.handler, \
        t->closure.p1, \
        t->metric.create_ms, \
        t->metric.done_ms, \
        t->metric.file, \
        t->metric.fun, \
        t->metric.line);

///
/// \brief ngx_http_rpc_destry_task
/// \param t
///

ngx_rpc_task_t *ngx_http_rpc_task_create(ngx_slab_pool_t *pool, ngx_log_t *log);

void ngx_http_rpc_task_destory(void *t);

void ngx_http_rpc_task_set_bufs(ngx_slab_pool_t *pool, ngx_chain_t *req_bufs, ngx_chain_t* src_bufs);



#endif
