#include "ngx_rpc_channel.h"
#include "ngx_http_rpc.h"


/// Rpc channel
///
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

    this->req = req;
    this->res = res;
    this->currenthandler = done;

    // a new task
    if(task && task->type == PROCESS_IN_PROC)
    {
        ngx_http_rpc_ctx_t *c = (ngx_http_rpc_ctx_t*)task->ctx;

        c->r_ctx = this;

        task = ngx_http_rpc_sub_request_task_init(r, c);

        task->type = PROCESS_IN_PROC;

        task->filter = ngx_http_inspect_application_subrequest_begin;

        task->path = ngx_slab_alloc_locked(task->pool, path.size() +1);

        strncpy(task->path, path.c_str(), path.size());

        NgxShmChainBufferWriter writer(&task->req_bufs, task->pool);
        req->SerializeToZeroCopyStream(&writer);

        ngx_rpc_notify_task(c->notify, NULL, task);
        return;
    }

}


static void RpcChannel::finish_request(void* ctx, ngx_rpc_task_t *task)
{

    RpcChannel *channel = (RpcChannel *)c->r_ctx;

    if(task->response_states == NGX_OK)
    {
            NgxChainBufferReader reader(&task->res_bufs);
            channel->res->SerializeToZeroCopyStream(&reader);
    }

    ngx_http_rpc_ctx_t *c = (ngx_http_rpc_ctx_t*) ctx;

   this->currenthandler(channel,channel->req, channel->res, task->response_states);
}
