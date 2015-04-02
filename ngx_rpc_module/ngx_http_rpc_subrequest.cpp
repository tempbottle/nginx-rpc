extern "C" {
    #include "ngx_http_rpc.h"
}

#include "ngx_http_rpc_subrequest.h"
#include "ngx_rpc_channel.h"

int ngx_http_header_modify_content_length(ngx_http_request_t *r, ngx_int_t value)
{
    r->headers_in.content_length_n = value;


    ngx_list_part_t *part =  &r->headers_in.headers.part;
    ngx_table_elt_t *header = (ngx_table_elt_t *)part->elts;
    unsigned int i = 0;
    for( i = 0; /* void */ ; i++)
    {
        if(i >= part->nelts)
        {
            if( part->next  == NULL)
            {
                break;
            }

            part = part->next;
            header = (ngx_table_elt_t *)part->elts;
            i = 0;
        }

        if(header[i].hash == 0)
        {
            continue;
        }

        if(0 == ngx_strncasecmp(header[i].key.data,
                                (u_char*)"Content-Length:",
                                header[i].key.len))
        {

            header[i].value.data = (u_char *)ngx_palloc(r->pool, 32);
            header[i].value.len = 32;

            snprintf((char*)header[i].value.data,
                     32, "%ld", value);
            r->headers_in.content_length->value =  header[i].value;

            return NGX_OK;
        }
    }
    return NGX_ERROR;
}


ngx_int_t ngx_http_rpc_subrequest_done(ngx_http_request_t *r,
    void *data, ngx_int_t rc)
{
    ngx_rpc_task_t *task = (ngx_rpc_task_t *)data;

    ngx_http_request_t* child_req = r;
    //ngx_http_request_t* parent_req = r->parent;

    task->response_states  = child_req->headers_out.status;


    if(child_req->headers_out.status != NGX_HTTP_OK)
    {
        ngx_http_upstream_t* rup = r->upstream;

        ngx_chain_t* req_chain = &task->req_bufs;

        for(ngx_chain_t* c = rup->out_bufs; c; c=c->next)
        {
            int buf_size = c->buf->last - c->buf->pos;
            req_chain->buf = (ngx_buf_t*)ngx_slab_alloc_locked(task->pool,
                                                               sizeof(ngx_buf_t));

            memcpy(c->buf, req_chain->buf,sizeof(ngx_buf_t));

            req_chain->buf->pos = req_chain->buf->start =
                    (u_char*) ngx_slab_alloc_locked(task->pool, buf_size);

            memcpy(c->buf->pos, req_chain->buf->pos, buf_size);
            req_chain->next = (ngx_chain_t*)ngx_slab_alloc_locked(task->pool,
                                                                  sizeof(ngx_chain_t));
            req_chain = req_chain->next;
            req_chain->next = NULL;
        }
    }

    task->filter = RpcChannel::finish_request;

    ngx_http_rpc_dispatcher_task(task);

    return NGX_DONE;
}


void ngx_http_rpc_subrequest_start(void *ctx, ngx_rpc_task_t *task)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t*)ctx;
    ngx_http_request_t* r = (ngx_http_request_t*)rpc_ctx->r;

    ngx_http_post_subrequest_t *psr = (ngx_http_post_subrequest_t *)
            ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));

    if(psr == NULL)
    {
        //TODO finish the request
        return;
    }

    psr->data = task;
    psr->handler = ngx_http_rpc_subrequest_done;

    r->request_body->bufs = &task->req_bufs;
    ngx_http_header_modify_content_length(r, task->res_length);

    ngx_str_t forward = ngx_string(task->path);

    ngx_http_request_t *sr;
    ngx_int_t rc = ngx_http_subrequest(r, &forward, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);

    if(rc != NGX_OK)
    {
        ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "ngx_http_subrequest failed:%d", rc);
        //delete  ctx;
        //handler(this, req, res, NGX_HTTP_INTERNAL_SERVER_ERROR);
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }
}

ngx_rpc_task_t* ngx_http_rpc_sub_request_task_init(ngx_http_request_t *r, void * ctx)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_rpc_module);

    // 1 new process task
    ngx_rpc_task_t* task = ngx_http_rpc_task_create(rpc_ctx->shpool, rpc_ctx);

    task->ctx = ctx;
    task->status = TASK_INIT;


    // 2 copy the request bufs
    return task;

}
