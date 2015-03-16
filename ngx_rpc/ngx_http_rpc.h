#include "ngx_log_cpp.h"
#include "ngx_rpc_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>


typedef struct
{
    void *sub_req_ctx;

} ngx_http_rpc_ctx_t;


typedef struct
{
    uint64_t shm_size;
    ngx_slab_pool_t *shpool;


} ngx_http_rpc_conf_t;



ngx_int_t ngx_http_rpc_master_init(ngx_cycle_t *cycle){

}
void      ngx_http_rpc_master_exit(ngx_cycle_t *cycle){

}

ngx_int_t ngx_http_rpc_process_init(ngx_cycle_t *cycle){

}

void      ngx_http_rpc_process_exit(ngx_cycle_t *cycle){

}


/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {
    { ngx_string("ngx_rpc_conf_file"),
      NGX_HTTP_SRV_CONF | NGX_CONF_NOARGS,
      ngx_conf_set_path_slot,
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
