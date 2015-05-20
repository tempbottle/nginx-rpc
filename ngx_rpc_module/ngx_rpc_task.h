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


///
/// \brief The task_closure_t struct
///
typedef struct {
    void (*handler)(ngx_rpc_task_t* _this, void *p1);
    void* p1;
} task_closure_t;



///
/// \brief The ngx_rpc_task_s struct
///
struct ngx_rpc_task_s {

    // need for rpc when exec_in_nginx = true
    ngx_queue_t       node;
    ngx_rpc_notify_t *proc_notify;
    ngx_rpc_notify_t *done_notify;
    ngx_slab_pool_t  *share_pool;
    ngx_log_t        *log;

    // need for nginx when exec_in_nginx = false
    ngx_pool_t       *ngx_pool;

    int               exec_in_nginx:1;
    int               exec_upstream:1;
    int               exec_subrequest:1;

    ngx_uint_t time_out_ms;

    task_closure_t closure; // exec in proc_notify
    task_closure_t finish; // exec in done_notify


    // for sub request or upstream
    u_char path[MAX_PATH_NAME];
    ngx_queue_t params;

    // input
    ngx_chain_t req_bufs;
    ngx_int_t  req_length;

    // output
    ngx_chain_t res_bufs;
    ngx_int_t   res_length;

    // task ret
    int response_states;
};




///
/// \brief ngx_http_rpc_destry_task
/// \param t
///

ngx_rpc_task_t *ngx_http_create_rpc_task(ngx_slab_pool_t *share_pool,
                                         ngx_pool_t *ngx_pool,
                                         ngx_log_t *log, int exec_in_nginx);

///
/// \brief ngx_http_rpc_task_reset
/// \param task
/// \param pool
/// \param ngx_pool
/// \param log
/// \return
///
ngx_rpc_task_t* ngx_http_rpc_task_reset(ngx_rpc_task_t *task);


////
/// \brief ngx_http_rpc_task_destory
/// \param t
///
void ngx_http_rpc_task_destory(void *t);


///
/// \brief ngx_http_rpc_task_set_bufs
/// \param task
/// \param req_bufs
/// \param src_bufs
///
void ngx_http_rpc_task_set_bufs(ngx_rpc_task_t *task, ngx_chain_t *req_bufs, ngx_chain_t* src_bufs);

#endif
