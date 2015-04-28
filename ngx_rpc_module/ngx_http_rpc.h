#ifndef _NGX_HTTP_RPC_H_
#define _NGX_HTTP_RPC_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>



#include "ngx_rpc_queue.h"


#include "ngx_rpc_process.h"

#define  CACAHE_LINESIZE 64
// sysconf(_SC_LEVEL1_DCACHE_LINESIZE)

/// for rpc_task
#include "ngx_rpc_task.h"



 // for rpc_conf and module
typedef struct
{
    ngx_rpc_queue_t  *proc_queue;
    ngx_rpc_notify_t *notify;
    ngx_log_t* log;

} ngx_http_rpc_conf_t;

extern ngx_module_t ngx_http_rpc_module;


typedef struct {
   ngx_str_t name;
   void (*handler)(ngx_rpc_task_t *_this, void *p1);
} method_conf_t;


//
void ngx_http_rpc_request_finish(ngx_rpc_task_t* _this, void *ctx);

void ngx_http_rpc_request_foward(ngx_rpc_task_t* _this, void *ctx);



#endif
