

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


void RpcChannel::forward_request(const std::string& path,
                 const ::google::protobuf::Message* req,
                 ::google::protobuf::Message* res,
                 RpcCallHandler done)
{

    RpcChannel* new_channel = new RpcChannel(this->r);

    new_channel->req = (::google::protobuf::Message*)req;
    new_channel->res = res;
    new_channel->done = done;

    NgxShmChainBufferWriter writer(task->res_bufs, task->pool);

    bool ret = req->SerializeToZeroCopyStream(&writer);
    task->res_length = writer.totaly;


    // reuse this task
    task->finish.handler = ngx_http_rpc_request_foward;
    task->finish.p1 = r;

    task->closure.handler = ngx_http_rpc_request_foward_done;
    task->closure.p1      = new_channel;

    strncpy(task->path, path.c_str(), path.size());

    if(task->done_notify != NULL)
         ngx_rpc_notify_push_task(task->done_notify, node);

    //
}


void RpcChannel::finish_request(ngx_rpc_task_t *task,void* ctx)
{


}
