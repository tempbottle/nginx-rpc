#include "ngx_rpc_task.h"


////
/// \brief ngx_http_rpc_task_create
/// \param pool
/// \param ctx
/// \return
///
ngx_rpc_task_t* ngx_http_rpc_task_create(ngx_slab_pool_t *pool, ngx_log_t *log)
{

    ngx_rpc_task_t* task = (ngx_rpc_task_t*)
            ngx_slab_alloc(pool, sizeof(ngx_rpc_task_t));

    if(task == NULL)
        return NULL;

    memset(task, 0, sizeof(ngx_rpc_task_t));
    task->log = log;
    return task;
}



///
/// \brief ngx_http_rpc_destry_task
/// \param t
///
void ngx_http_rpc_task_destory(ngx_rpc_task_t *t){

    ngx_slab_pool_t *pool = t->pool;

    // free params

    // free req_bufs
    {
        ngx_chain_t * ptr = &t->req_bufs;
        for( ;ptr->buf != NULL;ptr= ptr->next)
            ngx_slab_free(pool, ptr->buf->start);
    }

    // free res_bufs
    {
        ngx_chain_t * ptr = &t->res_bufs;
        for( ;ptr->buf != NULL;ptr= ptr->next)
            ngx_slab_free(pool, ptr->buf->start);
    }

    // free task
    ngx_slab_free(pool, t);
}


