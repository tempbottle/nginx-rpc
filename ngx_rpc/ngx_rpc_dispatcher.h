#ifndef _NGX_RPC_DISPATCHER_H_
#define _NGX_RPC_DISPATCHER_H_

#include "ngx_rpc_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#define  NGX_RPC_SHM_NAME "NGX_RPC_DIS"

typedef struct ngx_http_rpc_conf_t
{
     int ngx_rpc_max_request_num;
     int ngx_rpc_max_request_buffer_size;
     ngx_shm_zone_t *shm_zone;

} ngx_http_rpc_conf;

// every
typedef struct ngx_http_rpc_ctx_t
{
    ngx_rpc_call_t* call;

} ngx_http_rpc_ctx;




#endif
