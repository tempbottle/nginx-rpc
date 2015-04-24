#ifndef __NGX_RPC_PROCESS_H__
#define __NGX_RPC_PROCESS_H__

#include <stdint.h>


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>


#include "ngx_rpc_task.h"
#include "ngx_rpc_queue.h"

// every process has its memery
typedef struct {
    ngx_slab_pool_t *shpool;
    ngx_rpc_queue_t * queue;

    ngx_rpc_notify_t* notify;

    ngx_uint_t                shm_size;
    ngx_log_t*                log;
} ngx_proc_rpc_conf_t;

extern ngx_module_t ngx_proc_rpc_module;

#define ngx_rpc_get_proc_conf() \



#endif
