#include "ngx_rpc_task.h"


////
/// \brief ngx_http_rpc_task_create
/// \param pool
/// \param ctx
/// \return
///
ngx_rpc_task_t* ngx_http_rpc_task_create(ngx_slab_pool_t *share_pool,
                                         ngx_pool_t *ngx_pool,
                                         ngx_log_t *log, int exec_in_nginx)
{

    ngx_rpc_task_t *task = NULL;

    if( exec_in_nginx )
    {
        task = (ngx_rpc_task_t*)ngx_palloc(ngx_pool, sizeof(ngx_rpc_task_t));
    }else{
        task = (ngx_rpc_task_t*)ngx_slab_alloc(pool,, sizeof(ngx_rpc_task_t));

    }

    if(task == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      " ngx_http_rpc_task_create failed");
        return NULL;
    }

    memset(task, 0, sizeof(ngx_rpc_task_t));

    task->log           = log;
    task->share_pool    = share_pool;
    task->ngx_pool      = ngx_pool;
    task->exec_in_nginx = exec_in_nginx;

    ngx_queue_init(&task->node);

    ngx_log_error(NGX_LOG_DEBUG, log, 0,
                  " ngx_http_rpc_task_create task:%p with node:%p", task, &task->node);

    return task;
}

ngx_rpc_task_t* ngx_http_rpc_task_reset(ngx_rpc_task_t *task, ngx_slab_pool_t *share_pool, ngx_pool_t *ngx_pool, ngx_log_t *log){

    ngx_http_rpc_task_set_bufs(task, &task->req_bufs, NULL);
    ngx_http_rpc_task_set_bufs(task, &task->res_bufs, NULL);

    int exec_in_nginx = task->exec_in_nginx;

    memset(task, 0, sizeof(ngx_rpc_task_t));

    task->log           = log;
    task->share_pool    = share_pool;
    task->ngx_pool      = ngx_pool;
    task->exec_in_nginx = exec_in_nginx;

    ngx_queue_init(&task->node);

    ngx_log_error(NGX_LOG_DEBUG, log, 0, " ngx_http_rpc_task_reset task:%p with node:%p",
                                          task, &task->node);

    return task;
}
///
/// \brief ngx_http_rpc_destry_task
/// \param t
///
void ngx_http_rpc_task_destory(void *t){

    ngx_rpc_task_t *task = t;


    // free params
    ngx_http_rpc_task_set_bufs(task, &task->req_bufs, NULL);
    ngx_http_rpc_task_set_bufs(task, &task->res_bufs, NULL);


    ngx_log_error(NGX_LOG_DEBUG, task->log, 0,
                  " ngx_http_rpc_task_destory task:%p with nodo:%p", task, &task->node);

    // free task
    if(!task->exec_in_nginx)
       ngx_slab_free(task->share_pool, task);
    else
       ngx_pfree(task->ngx_pool, task);
}

void ngx_http_rpc_task_set_bufs(ngx_rpc_task_t* task, ngx_chain_t* req_bufs, ngx_chain_t* src_bufs)
{
    if(task->exec_in_nginx)
    {
        ngx_chain_t t ={NULL, NULL};
        *req_bufs = src_bufs ? t : *src_bufs;
        return;
    }


    // head
    if(req_bufs->buf != NULL )
    {
        ngx_slab_free(task->share_pool, req_bufs->buf->pos);
        ngx_slab_free(task->ngx_pool, req_bufs->buf);
        req_bufs->buf = NULL;
    }

    // next
    ngx_chain_t** ptr = &(req_bufs->next);

    for( ; *ptr != NULL; ptr = &((*ptr)->next))
    {
        ngx_slab_free(task->share_pool, (*ptr)->buf->pos);
        ngx_slab_free(task->share_pool, (*ptr)->buf);
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

