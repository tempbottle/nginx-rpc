#include "ngx_rpc_task.h"

////
/// \brief ngx_http_rpc_task_create
/// \param pool
/// \param ctx
/// \return
///
ngx_rpc_task_t* ngx_http_rpc_task_create(ngx_slab_pool_t *pool, void *ctx)
{

    ngx_rpc_task_t* task = (ngx_rpc_task_t*)
            ngx_slab_alloc_locked(ctx->shpool, sizeof(ngx_rpc_task_t));

    memset(task, 0, sizeof(task));

    task->ctx = ctx;

    task->init_time_ms = ngx_current_msec;
    task->time_out_ms  = ngx_current_msec + ctx->timeout_ms;

    return task;
}


///
/// \brief ngx_http_rpc_destry_task
/// \param t
///
void ngx_http_rpc_task_destory(ngx_rpc_task_t *t){

    ngx_slab_pool_t *pool = t->pool;

    if(t->path)
        ngx_slab_free_locked(pool, t->path);

    // free params

    // free req_bufs

    // free res_bufs

    // free task

}

