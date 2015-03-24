#include "ngx_rpc_server.h"
#include "ngx_http_rpc.h"


int ngx_http_header_modify_content_length(ngx_http_request_t *r, ngx_int_t value)
{
    r->headers_in.content_length_n = value;


    ngx_list_part_t *part =  &r->headers_in.headers.part;
    ngx_table_elt_t *header = part->elts;
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
            header = part->elts;
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

            header[i].value.data = ngx_palloc(r->pool, 32);
            header[i].value.len = 32;

            snprintf((char*)header[i].value.data,
                     32, "%ld", value);
            r->headers_in.content_length->value =  header[i].value;

            return NGX_OK;
        }
    }
    return NGX_ERROR;
}




static void RpcChannel::destructor(void *p)
{
    RpcChannel *tp = (RpcChannel*)p;
    delete tp->req;
    delete tp->res;
    delete tp;
}


void RpcChannel::sub_request(const std::string& path,
                 const ::google::protobuf::Message* req,
                 ::google::protobuf::Message* res,
                 RpcCallHandler done)
{

    ngx_rpc_task_t *task = NULL;

    // a new task
    if(task && task->type == PROCESS_IN_PROC)
    {
        ngx_http_rpc_ctx_t *c = (ngx_http_rpc_ctx_t*)task->ctx;

        task = ngx_http_rpc_sub_request_task_init(r, c);
        task->type = PROCESS_IN_PROC;
        task->path = ngx_slab_alloc_locked(task->pool, path.size() +1);
        strncpy(task->path, path.c_str(), path.size());
        ngx_rpc_notify_task(c->notify, NULL, task);
        return;
    }


}
