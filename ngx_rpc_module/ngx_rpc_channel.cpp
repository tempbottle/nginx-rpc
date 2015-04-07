

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

    //ngx_rpc_task_t *task = NULL;

    this->req = (::google::protobuf::Message*)req;
    this->res = res;
    this->currenthandler = done;

    // a new task

}


void RpcChannel::finish_request(void* ctx, ngx_rpc_task_t *task)
{

     /*

    RpcChannel *channel = (RpcChannel *)c->r_ctx;

    if(task->response_states == NGX_OK)
    {
        NgxShmChainBufferWriter writer(task->res_bufs, task->pool);
        channel->res->SerializeToZeroCopyStream(&writer);
    }


   channel->currenthandler(channel,channel->req, channel->res, task->response_states);
   */
}
