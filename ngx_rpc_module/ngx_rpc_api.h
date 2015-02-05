/** this file use in nginx*/
#ifndef _NGX_RPC_API_H_
#define _NGX_RPC_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>


typedef struct ngx_http_rpc_ctx_t
{

} ngx_http_rpc_ctx;


typedef struct ngx_http_rpc_conf_t
{
    ngx_str_t json_conf_path; // the conf path
} ngx_http_rpc_conf;


//ngx_int_t           (*init_module)(ngx_cycle_t *cycle);
ngx_int_t ngx_http_rpc_master_init(ngx_cycle_t *cycle);
void      ngx_http_rpc_master_exit(ngx_cycle_t *cycle);

ngx_int_t ngx_http_rpc_process_init(ngx_cycle_t *cycle);
void      ngx_http_rpc_process_exit(ngx_cycle_t *cycle);


extern ngx_module_t ngx_http_rpc_module;

void ngx_http_rpc_post_hander(ngx_http_request_t *r);

#ifdef __cplusplus
}
#endif

#endif //_NGX_RPC_API_H_
