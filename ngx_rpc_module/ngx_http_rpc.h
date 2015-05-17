#ifndef _NGX_HTTP_RPC_H_
#define _NGX_HTTP_RPC_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>



#include "ngx_rpc_queue.h"


#include "ngx_rpc_process.h"

#define  CACAHE_LINESIZE 64
//sysconf(_SC_LEVEL1_DCACHE_LINESIZE)

/// for rpc_task
#include "ngx_rpc_task.h"

#define NGX_HTTP_RPC_METHOD_MAX 256
typedef struct {
   ngx_log_t *log;
   ngx_str_t  name;
   void      *_impl; // server instance
   void     (*handler)(ngx_rpc_task_t *_this, void *p1);// wrapper of rpc server impl
   int       exec_in_nginx;
} method_conf_t;

 // for rpc_conf and module
typedef struct
{
    ngx_rpc_queue_t  *proc_queue;
    ngx_rpc_notify_t *notify;

    ngx_array_t       *method_array;
    ngx_hash_t        *method_hash;

} ngx_http_rpc_main_conf_t;

typedef struct {
    ngx_http_upstream_conf_t upstream;
} ngx_http_rpc_loc_conf_t;



typedef struct {
    ngx_http_rpc_main_conf_t *rpc_conf;
    method_conf_t       *method;

    ngx_http_request_t  *r;
    ngx_rpc_task_t      *task;
    ngx_slab_pool_t     *pool;

    ngx_http_status_t    status;
} ngx_http_rpc_ctx_t;

void ngx_http_rpc_ctx_destroy(void *p);



extern ngx_module_t ngx_http_rpc_module;


// for sub request
void ngx_http_rpc_request_finish(ngx_rpc_task_t* _this, void *ctx);

void ngx_http_rpc_request_foward(ngx_rpc_task_t* _this, void *ctx);



// for upstream
void ngx_http_rpc_start_upsteam(ngx_rpc_task_t* _this, void *ctx);



#endif
