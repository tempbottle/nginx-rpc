

extern "C" {
#include "ngx_http_rpc.h"
}


#include "ngx_rpc_channel.h"

/// Rpc channel
///
void RpcChannel::destructor(void *p)
{
    RpcChannel *tp = (RpcChannel*)p;
    delete tp->req;
    delete tp->res;
    delete tp;
}



void RpcChannel::foward_done(ngx_rpc_task_t* task, void *ctx)
{
    RpcChannel *new_channel = (RpcChannel *)ctx;

    NgxChainBufferReader reader(task->req_bufs);

    bool ret = new_channel->res->ParseFromZeroCopyStream(&reader);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, " foward_done task:%p, status:%d parse:%d", task, task->response_states, ret);

    new_channel->done(new_channel->pre_cntl, new_channel->req, new_channel->res, task->response_states);

    delete new_channel;
}

void RpcChannel::forward_request(const std::string& path,
                 const ::google::protobuf::Message* req,
                 ::google::protobuf::Message* res,
                 RpcCallHandler done)
{

    RpcChannel* new_channel = new RpcChannel(this->r);
    new_channel->pre_cntl = this;

    new_channel->req = (::google::protobuf::Message*)req;
    new_channel->res = res;
    new_channel->done = done;

    NgxShmChainBufferWriter writer(task->res_bufs, task->pool);

    bool ret = req->SerializeToZeroCopyStream(&writer);
    task->res_length = writer.totaly;


    // reuse this task
    task->finish.handler = ngx_http_rpc_request_foward;
    task->finish.p1 = r;

    task->closure.handler = RpcChannel::foward_done;
    task->closure.p1      = new_channel;

    strncpy((char*)task->path, path.c_str(), path.size());

    if(task->done_notify != NULL)
         ngx_rpc_notify_push_task(task->done_notify, &task->node);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                  "forward_request SerializeToZeroCopyStream:%d res_length:%d nofity:%p path:%s",
                  ret, task->res_length, task->done_notify, task->path);

}

void RpcChannel::finish_request(RpcChannel *channel, const google::protobuf::Message *req, google::protobuf::Message *res, int result)
{
    ngx_rpc_task_t *task = channel->task;
    task->response_states = result;
    NgxShmChainBufferWriter writer(task->res_bufs, task->pool);
    int ex = res->ByteSize();
    bool ret = res->SerializeToZeroCopyStream(&writer);
    task->res_length = writer.totaly;

    // push the done task
    if(task->done_notify != NULL)
    {
        ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                      "SerializeToZeroCopyStream:%d size:%d,expect:%d notify eventfd:%d",
                      ret, task->res_length, ex, task->done_notify->event_fd);
        // done
        task->finish.handler = ngx_http_rpc_request_finish;
        task->finish.p1 = channel->r;
        ngx_rpc_notify_push_task(task->done_notify, &(task->node));
    }

    // clean the channel
    delete channel;
}


