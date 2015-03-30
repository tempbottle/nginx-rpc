

extern "C" {
#include "ngx_http_rpc.h"
}


#include "ngx_rpc_channel.h"
#include "ngx_http_rpc_subrequest.h"
/// Rpc channel
///
void RpcChannel::destructor(void *p)
{
    RpcChannel *tp = (RpcChannel*)p;
    delete tp->req;
    delete tp->res;
    delete tp;
}


void RpcChannel::start_subrequest(const std::string& path,
                 const ::google::protobuf::Message* req,
                 ::google::protobuf::Message* res,
                 RpcCallHandler done)
{

    ngx_rpc_task_t *task = NULL;

    this->req = (::google::protobuf::Message*)req;
    this->res = res;
    this->currenthandler = done;

    // a new task
    if(task && task->type == PROCESS_IN_PROC)
    {
        ngx_http_rpc_ctx_t *c = (ngx_http_rpc_ctx_t*)task->ctx;

        c->r_ctx = this;

        task = ngx_http_rpc_sub_request_task_init(r, c);

        task->type = PROCESS_IN_PROC;

        task->filter = ngx_http_rpc_subrequest_start;

        task->path = (char *)ngx_slab_alloc_locked(task->pool, path.size() +1);

        strncpy(task->path, path.c_str(), path.size());

        NgxShmChainBufferWriter writer(task->req_bufs, task->pool);
        req->SerializeToZeroCopyStream(&writer);

        ngx_rpc_notify_task(c->notify, NULL, task);
        return;
    }
}


void RpcChannel::finish_request(void* ctx, ngx_rpc_task_t *task)
{

    ngx_http_rpc_ctx_t *c = (ngx_http_rpc_ctx_t*) ctx;

    RpcChannel *channel = (RpcChannel *)c->r_ctx;

    if(task->response_states == NGX_OK)
    {
        NgxShmChainBufferWriter writer(task->res_bufs, task->pool);
        channel->res->SerializeToZeroCopyStream(&writer);
    }


   channel->currenthandler(channel,channel->req, channel->res, task->response_states);
}
