#ifndef _NGX_HTTP_RPC_H_
#define _NGX_HTTP_RPC_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#include "ngx_rpc_notify.h"

#include "ngx_rpc_queue.h"


#include "ngx_rpc_process.h"

#define  CACAHE_LINESIZE 64
#define  MAX_TASK_STACK  16

// sysconf(_SC_LEVEL1_DCACHE_LINESIZE)

/// for rpc_task
#include "ngx_rpc_task.h"


ngx_rpc_task_t* ngx_http_rpc_post_request_task_init(ngx_http_request_t *r, void * ctx);

ngx_rpc_task_t* ngx_http_rpc_sub_request_task_init(ngx_http_request_t *r, void * ctx);

///
/// \brief ngx_http_rpc_proccess_task
/// \param task
///
//static void ngx_http_rpc_proccess_task(ngx_rpc_task_t* task);

///
/// \brief ngx_http_rpc_dispatcher_task
/// \param task
///
void ngx_http_rpc_dispatcher_task(ngx_rpc_task_t* task);



/// for rpc_ctx
typedef struct
{
    ngx_slab_pool_t  *shpool;
    ngx_rpc_notify_t *notify;

    ngx_http_request_t *r;
    void* r_ctx;

    ngx_shmtx_sh_t sh_lock;
    ngx_shmtx_t task_lock;

    ngx_queue_t pending;
    ngx_queue_t done;

    uint64_t timeout_ms;
    ngx_log_t *log;

} ngx_http_rpc_ctx_t;

///
/// \brief ngx_http_rpc_ctx_init
/// \param r
/// \param ctx
/// \return
///
ngx_http_rpc_ctx_t*  ngx_http_rpc_ctx_init(ngx_http_request_t *r, void *ctx);
void ngx_http_rpc_ctx_finish_by_task(void *ctx);
void ngx_http_rpc_ctx_free(void* ctx);



 // for rpc_conf and module
typedef struct
{
    ngx_uint_t request_capacity;
    ngx_uint_t shm_size;

    // create by master shared by all process
    ngx_slab_pool_t  *shpool;
    ngx_rpc_notify_t *notify;

} ngx_http_rpc_conf_t;

extern ngx_module_t ngx_http_rpc_module;


#endif
