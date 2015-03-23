#ifndef _NGX_HTTP_RPC_H_
#define _NGX_HTTP_RPC_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#include "ngx_rpc_notify.h"
#include "ngx_rpc_task.h"
#include "ngx_rpc_queue.h"


#include "ngx_rpc_process.h"

#define  CACAHE_LINESIZE 64
#define  MAX_TASK_STACK  16

// sysconf(_SC_LEVEL1_DCACHE_LINESIZE)

// dispatcher of nginx taskd

// shared the data of request for dispatcher


typedef struct
{
    ngx_slab_pool_t  *shpool;
    ngx_rpc_notify_t *notify;

    ngx_http_request_t *r;
    void* r_ctx;

    ngx_shmtx_sh_t sh_lock;
    ngx_shmtx_t task_lock;

    ngx_queue_t pending;
    ngx_queue_t done;

    uint64_t timeout_ms;

} ngx_http_rpc_ctx_t;


 void ngx_http_rpc_ctx_free(void* ctx);


typedef struct
{
    ngx_uint_t request_capacity;
    ngx_uint_t shm_size;

    // create by master shared by all process
    ngx_slab_pool_t  *shpool;
    ngx_rpc_notify_t *notify;
} ngx_http_rpc_conf_t;



extern ngx_module_t ngx_http_rpc_module;


/// schedule the request
void ngx_http_rpc_yield_request();
void ngx_http_rpc_resume_request();




ngx_http_rpc_ctx_t* ngx_http_rpc_ctx_init(ngx_http_request_t *r, void *ctx);


ngx_rpc_task_t* ngx_http_rpc_post_request_task_init(ngx_http_request_t *r,void * ctx)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_rpc_module);

    // 1 new process task
    ngx_rpc_task_t* task = ngx_http_rpc_task_create(rpc_ctx->shpool, rpc_ctx);

    task->ctx = ctx;
    task->status = TASK_INIT;

    // 2 copy the request bufs

    ngx_chain_t* req_chain = &task->req_bufs;

    for(ngx_chain_t* c= r->request_body->bufs; c; c=c->next )
    {
        int buf_size = c->buf->last - c->buf->pos;
        req_chain->buf = (ngx_buf_t*)ngx_slab_alloc_locked(rpc_ctx->shpool,
                                                           sizeof(ngx_buf_t));

        memcpy(c->buf, req_chain->buf,sizeof(ngx_buf_t));

        req_chain->buf->pos = req_chain->buf->start =
                (u_char*) ngx_slab_alloc_locked(ctx->shpool,buf_size);

        memcpy(c->buf->pos, req_chain->buf->pos,buf_size);
        req_chain->next = (ngx_chain_t*)ngx_slab_alloc_locked(ctx->shpool,
                                                              sizeof(ngx_chain_t));
        req_chain = req_chain->next;
        req_chain->next = NULL;
    }

    return task;

}

#endif
