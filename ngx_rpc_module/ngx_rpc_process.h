#ifndef __NGX_RPC_PROCESS_H__
#define __NGX_RPC_PROCESS_H__

#include <stdint.h>


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#include "ngx_rpc_task.h"
int ngx_rpc_process_push_task(ngx_rpc_task_t *task);


#endif
