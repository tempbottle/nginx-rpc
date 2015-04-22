#include "ngx_rpc_task.h"


////
/// \brief ngx_http_rpc_task_create
/// \param pool
/// \param ctx
/// \return
///
ngx_rpc_task_t* ngx_http_rpc_task_create(ngx_slab_pool_t *pool, ngx_log_t *log)
{

    ngx_rpc_task_t *task = (ngx_rpc_task_t*)
            ngx_slab_alloc(pool, sizeof(ngx_rpc_task_t));

    if(task == NULL)
        return NULL;

    memset(task, 0, sizeof(ngx_rpc_task_t));
    task->log = log;
    task->pool = pool;

    ngx_queue_init(&task->done);

    return task;
}

///
/// \brief ngx_http_rpc_destry_task
/// \param t
///
void ngx_http_rpc_task_destory(void *t){

    ngx_rpc_task_t * task = t;
    ngx_slab_pool_t *pool = task->pool;

    // free params

    // free req_bufs
    {
        ngx_chain_t * ptr = &task->req_bufs;
        for( ; ptr && ptr->buf != NULL; ptr = ptr->next)
            ngx_slab_free(pool, ptr->buf->start);
    }

    // free res_bufs
    {
        ngx_chain_t * ptr = &task->res_bufs;
        for( ;ptr && ptr->buf != NULL;ptr= ptr->next)
            ngx_slab_free(pool, ptr->buf->start);
    }

    // free task
    ngx_slab_free(pool, task);
}


ngx_rpc_task_queue_t *ngx_http_rpc_task_queue_create(ngx_slab_pool_t *pool)
{
    ngx_rpc_task_queue_t *q = ngx_slab_alloc(pool, sizeof(ngx_rpc_task_queue_t));

    q->pool = pool;

    ngx_queue_init(&q->next);

    if(ngx_shmtx_create(&q->next_lock, &q->next_sh, NULL) != NGX_OK)
    {
        ngx_slab_free(pool, q);
        return NULL;
    }

    return q;
}


void ngx_http_rpc_task_queue_destory(ngx_rpc_task_queue_t *q)
{
    ngx_slab_free(q->pool, q);
}
