#ifndef _NGX_RPC_QUEUE_H_
#define _NGX_RPC_QUEUE_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

typedef void(*ngx_task_hanlder)(void * );


#define NGX_RPC_TASK_INIT 0
#define NGX_RPC_TASK_SUBREQUST 1
#define NGX_RPC_TASK_SUBREQUST_DONE -1
#define NGX_RPC_TASK_DONE -1

typedef struct {
    ngx_buf_t shm_req_buf;
    ngx_array_t shm_req_header;

    ngx_buf_t  shm_res_buf;
    ngx_array_t  shm_res_header;

    ngx_task_hanlder hander;
    int response_states;
} ngx_rpc_task_t;


#define MAX_RPC_CALL_NUM 2

typedef struct
{
    ngx_rpc_task_t task[MAX_RPC_CALL_NUM];
    int sp;

    ngx_http_request_t *r;
    int eventfd;

    ngx_slab_pool_t *shpool;

    ngx_rpc_call_t *pending;

} ngx_rpc_call_t;





#ifdef __cplusplus
}
#endif






#endif
