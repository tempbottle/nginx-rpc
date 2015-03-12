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





// associate the http request
typedef struct
{
    void *sub_req_ctx;


} ngx_http_rpc_ctx_t;




typedef struct ngx_http_rpc_conf_t
{
    ngx_str_t json_conf_path; // the conf path

} ngx_http_rpc_conf;


// for rpc server
//ngx_int_t (*init_module)(ngx_cycle_t *cycle);
ngx_int_t ngx_http_rpc_master_init(ngx_cycle_t *cycle);
void      ngx_http_rpc_master_exit(ngx_cycle_t *cycle);

ngx_int_t ngx_http_rpc_process_init(ngx_cycle_t *cycle);
void      ngx_http_rpc_process_exit(ngx_cycle_t *cycle);


extern ngx_module_t ngx_http_rpc_module;
void ngx_http_rpc_post_handler(ngx_http_request_t *r);


// for rpc client
typedef void(*ngx_rpc_post_handler_pt)(void *ctx);
int ngx_rpc_post_task(ngx_rpc_post_handler_pt callback, void *data);


typedef struct {
    ngx_rpc_post_handler_pt handler;
    void *ctx;
} ngx_notify_queue_t ;

extern ngx_notify_queue_t ngx_rpc_post_start;
extern int _ngx_main(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif //_NGX_RPC_API_H_
