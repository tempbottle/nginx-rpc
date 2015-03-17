#include "ngx_log_cpp.h"
#include "ngx_rpc_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>



#define  CACAHE_LINESIZE 64
// sysconf (_SC_LEVEL1_DCACHE_LINESIZE)
typedef void(*ngx_task_hanlder)(void * );


#define NGX_RPC_TASK_INIT 0
#define NGX_RPC_TASK_SUBREQUST_INIT  1
#define NGX_RPC_TASK_SUBREQUST_DONE -1
#define NGX_RPC_TASK_DONE -1

typedef struct {
    ngx_chain_t req_bufs;
    ngx_chain_t res_bufs;
    ngx_task_hanlder hander;
    void * ctx;
    int response_states;
} ngx_rpc_task_t;


#define MAX_RPC_CALL_NUM  2
#define MAX_PENDING_LEVEL 5


typedef struct
{
    ngx_rpc_task_t task[MAX_RPC_CALL_NUM];
    int sp;

    ngx_http_request_t *r;
    ngx_slab_pool_t *shpool;
    pending;
    ngx_shmtx_t shm_lock;

} ngx_rpc_call_t;


typedef struct
{
    void *sub_req_ctx;

} ngx_http_rpc_ctx_t;


typedef struct
{
    uint64_t max_request_cpu_radio;
    uint64_t shm_size;
    ngx_slab_pool_t *shpool;
    ngx_rpc_queue_t *queue;


} ngx_http_rpc_conf_t;



ngx_int_t ngx_http_rpc_master_init(ngx_cycle_t *cycle)
{

}


void      ngx_http_rpc_master_exit(ngx_cycle_t *cycle)
{

}

ngx_int_t ngx_http_rpc_process_init(ngx_cycle_t *cycle){


}

void      ngx_http_rpc_process_exit(ngx_cycle_t *cycle)
{

}

static ngx_int_t ngx_proc_rpc_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_http_rpc_conf_t       *ctx, *octx;

    octx = data;
    ctx = shm_zone->data;

    if (octx != NULL) {
        if (ctx->shm_size !=  octx->shm_size) {
            ngx_log_error(NGX_LOG_EMERG, shm_zone->shm.log, 0,
                          "reqstat \"%V\" uses the value str \"%d\" "
                          "while previously it used \"%d\"",
                          &shm_zone->shm.name, ctx->shm_size, octx->shm_size);
            return NGX_ERROR;
        }

        ctx->shpool = octx->shpool;
        ctx->queue = octx->queue;
        ctx->max_request_cpu_radio = octx->max_request_cpu_radio;
        return NGX_OK;
    }

    ctx->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;
    ctx->shpool->data = ctx;

    if( 0 != ngx_rpc_queue_create(&ctx->queue, ngx_cycle, ctx->shpool))
        return NGX_ERROR;


    return NGX_OK;
}

static char * ngx_http_rpc_conf_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rpc_conf_t *c = ( ngx_http_rpc_conf_t *) conf;

    ngx_str_t  *value = cf->args->elts;
    ssize_t size = ngx_parse_size(&value[1]);

    if (size == NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid zone size \"%V\"", &value[3]);
        return NGX_CONF_ERROR;
    }

    if (size < (ssize_t) (8 * ngx_pagesize)) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "zone \"%V\" is too small", &value[1]);
        return NGX_CONF_ERROR;
    }

    static ngx_str_t ngx_http_rpc = ngx_string("ngx_http_rpc_shm");

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &ngx_http_rpc, size,
                &ngx_http_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = c;

   return NGX_CONF_OK;
}

/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {
    { ngx_string("max_request_cpu_radio"),
      NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_rpc_conf_t, max_request_cpu_radio),
      NULL },

    { ngx_string("ngx_http_rpc_shm_size"),
      NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
      ngx_http_rpc_conf_set_shm_size,
      NGX_HTTP_SRV_CONF_OFFSET,
      0,
      NULL },
    ngx_null_command
};




/* Modules */
static ngx_http_module_t  ngx_http_rpc_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL,                /* create main configuration */
    NULL,                /* init main configuration */

    ngx_http_soap_sub_create_srv_conf,/* create server configuration */
    NULL,                /* merge server configuration */

    NULL,                /* create location configuration */
    NULL                 /* merge location configuration */
};


ngx_module_t  ngx_http_rpc_module = {
    NGX_MODULE_V1,
    &ngx_http_rpc_module_ctx,              /* module context */
    ngx_http_rpc_module_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but some where called init module instead
    ngx_http_rpc_master_init,              /* init module */
    ngx_http_rpc_process_init,             /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_http_rpc_process_exit,             /* exit process */
    ngx_http_rpc_master_exit,             /* exit master */
    NGX_MODULE_V1_PADDING
};






#ifdef __cplusplus
}
#endif
