#ifndef _NGX_HTTP_RPC_H_
#define _NGX_HTTP_RPC_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#include "ngx_rpc_notify.h"
#include "ngx_rpc_task.h"
#include "ngx_rpc_queue.h"

#define  CACAHE_LINESIZE 64
// sysconf(_SC_LEVEL1_DCACHE_LINESIZE)



// shared the data of request for dispatcher
typedef struct
{
    ngx_slab_pool_t  *shpool;
    ngx_rpc_queue_t  *queue;
    ngx_rpc_notify_t *notify;


    ngx_http_request_t *r;
    void* r_ctx;

    ngx_shmtx_sh_t psh;
    ngx_shmtx_t task_lock;

    ngx_queue_t appendtask;



    ngx_rpc_task_t *pre_task;

    uint64_t timeout_ms;

} ngx_http_rpc_ctx_t;


 void ngx_http_rpc_ctx_free(void* ctx);


typedef struct
{
    ngx_uint_t max_request_pow_2;
    ngx_uint_t shm_size;

    // create by master shared by all process
    ngx_slab_pool_t  *shpool;
    ngx_rpc_queue_t  *queue;
    ngx_rpc_notify_t *notify;
} ngx_http_rpc_conf_t;



extern ngx_module_t ngx_http_rpc_module;


/// schedule the request
void ngx_http_rpc_yield_request();
void ngx_http_rpc_resume_request();


#endif
