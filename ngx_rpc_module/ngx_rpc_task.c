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

    ngx_queue_init(&task->node);

    ngx_log_error(NGX_LOG_DEBUG, log, 0,
                  " ngx_http_rpc_task_create task:%p with nodo:%p", task, &task->node);

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
    ngx_log_error(NGX_LOG_DEBUG, task->log, 0,
                  " ngx_http_rpc_task_destory task:%p with nodo:%p", task, &task->node);

    // free task
    ngx_slab_free(pool, task);


}

void ngx_http_rpc_task_set_bufs(ngx_slab_pool_t *pool, ngx_chain_t* req_bufs, ngx_chain_t* src_bufs)
{
    // head
    if(req_bufs->buf != NULL )
    {
        ngx_slab_free(pool, req_bufs->buf->pos);
        ngx_slab_free(pool, req_bufs->buf);
        req_bufs->buf = NULL;
    }

    // next
    ngx_chain_t** ptr = &(req_bufs->next);

    for( ; *ptr != NULL; ptr = &((*ptr)->next))
    {
        ngx_slab_free(pool, (*ptr)->buf->pos);
        ngx_slab_free(pool, (*ptr)->buf);
        (*ptr)->buf = NULL;

        ngx_slab_free(pool, *ptr);
    }

    // clear
    ptr = &req_bufs;
    (*ptr)->next  = NULL;

    for(; src_bufs != NULL && src_bufs->buf != NULL ; src_bufs = src_bufs->next )
    {
        ngx_uint_t size = src_bufs->buf->last - src_bufs->buf->pos;

        (*ptr)->buf = (ngx_buf_t*)ngx_slab_alloc(pool, sizeof(ngx_buf_t));
        memcpy((*ptr)->buf, src_bufs->buf, sizeof(ngx_buf_t));

        (*ptr)->buf->pos = (*ptr)->buf->start = (u_char*) ngx_slab_alloc(pool, size);
        memcpy( (*ptr)->buf->pos, src_bufs->buf->pos, size);

        (*ptr)->buf->last =(*ptr)->buf->end = (*ptr)->buf->pos + size;

        if(src_bufs->next == NULL)
        {
            (*ptr)->next = NULL;
            break;
        }

        // new node
        (*ptr)->next = (ngx_chain_t*)ngx_slab_alloc(pool, sizeof(ngx_chain_t));
        ptr = &((*ptr)->next);
    }

}
