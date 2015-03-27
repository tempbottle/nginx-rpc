#ifndef _NGX_HTTP_RPC_SUBREQUEST_H_
#define _NGX_HTTP_RPC_SUBREQUEST_H_

extern "C" {
    #include "ngx_rpc_task.h"
}

// for sub request

int ngx_http_header_modify_content_length(ngx_http_request_t *r, ngx_int_t value);
void ngx_http_rpc_subrequest_start(void* ctx, ngx_rpc_task_t *task);
void ngx_http_rpc_subrequest_done(void* ctx, ngx_rpc_task_t *task);
#endif