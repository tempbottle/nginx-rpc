
extern "C" {
#include "ngx_http_rpc.h"
}


#include "ngx_rpc_channel.h"

void RpcChannel::upstream(const std::string& upstream_path,
                          const ::google::protobuf::Message* req,
                          ::google::protobuf::Message* res,
                          RpcCallHandler done)
{

    if(task->exec_subrequest || task->exec_subrequest)
    {

        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                      " ngx_rpc_task:%p is busy exec_upstream:%d exec_subrequest:%d you should call this in RpcCallHandler done ",
                      task, task->exec_upstream, task->exec_subrequest);
        done(this, req, res, NGX_HTTP_SERVICE_UNAVAILABLE);
        return;
    }

    this->task->exec_upstream = 1;

    // new task, new cntl
    RpcChannel* rpc_call_channel = new RpcChannel(this->r);

    rpc_call_channel->req  = req;
    rpc_call_channel->res  = res;
    rpc_call_channel->done = done;

    // only one upstream can do in same time
    rpc_call_channel->task =  this->task;

    if(upsteam_task->exec_in_nginx)
    {
        NgxChainBufferWriter  writer(task->r, task->ngx_pool);

        req->SerializeToZeroCopyStream(&writer);
        upsteam_task->req_length = writer.totaly;

    }else{

        NgxShmChainBufferWriter  writer(task->r, task->ngx_pool);

        req->SerializeToZeroCopyStream(&writer);
        upsteam_task->req_length = writer.totaly;
    }

    task->finish.hanlder = ngx_http_rpc_start_upsteam;
    task->finish.p1      = this->r;

    task->closure.hanlder = RpcChannel::rpc_call_done;
    task->closure.p1      = rpc_call_channel;

    strncpy((char*)task->path, path.c_str(), path.size());

    if(this->task->exec_in_nginx)
    {
        task->finish.handler(task, task->finish.p1);
    }else{
        ngx_rpc_notify_push_task(task->finish, &task->node);
    }
}

static void RpcChannel::upstream_done(ngx_rpc_task_t* task, void *ctx)
{
    this->task->exec_upstream = 0;
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

    if(task->exec_subrequest || task->exec_subrequest)
    {

        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                      " ngx_rpc_task:%p is busy exec_upstream:%d exec_subrequest:%d you should call this in RpcCallHandler done ",
                      task, task->exec_upstream, task->exec_subrequest);
        done(this, req, res, NGX_HTTP_SERVICE_UNAVAILABLE);
        return;
    }

    RpcChannel* new_channel = new RpcChannel(this->r);
    new_channel->pre_cntl = this;

    new_channel->req = (::google::protobuf::Message*)req;
    new_channel->res = res;
    new_channel->done = done;

    if(upsteam_task->exec_in_nginx)
    {
        NgxChainBufferWriter  writer(task->r, task->ngx_pool);
        req->SerializeToZeroCopyStream(&writer);
        upsteam_task->req_length = writer.totaly;

    }else{

        NgxShmChainBufferWriter  writer(task->r, task->ngx_pool);
        req->SerializeToZeroCopyStream(&writer);
        upsteam_task->req_length = writer.totaly;
    }

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


void RpcChannel::forward_done(ngx_rpc_task_t* task, void *ctx)
{
    this->task->exec_upstream = 0;

    RpcChannel *new_channel = (RpcChannel *)ctx;

    NgxChainBufferReader reader(task->req_bufs);

    bool ret = new_channel->res->ParseFromZeroCopyStream(&reader);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, " foward_done task:%p, status:%d parse:%d", task, task->response_states, ret);

    new_channel->done(new_channel->pre_cntl, new_channel->req, new_channel->res, task->response_states);

    delete new_channel;
}



void RpcChannel::finish_request(RpcChannel *channel, const google::protobuf::Message *req, google::protobuf::Message *res, int result)
{
    ngx_rpc_task_t *task = channel->task;
    task->response_states = result;
    NgxShmChainBufferWriter writer(task->res_bufs, task->pool);
    int ex = res->ByteSize();
    bool ret = res->SerializeToZeroCopyStream(&writer);
    task->res_length = writer.totaly;

    task->finish.handler = ngx_http_rpc_request_finish;
    task->finish.p1 = channel->r;


    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                  "SerializeToZeroCopyStream:%d size:%d,expect:%d notify eventfd:%d",
                  ret, task->res_length, ex, task->done_notify->event_fd);


    if(task->exec_in_nginx)
    {
        task->finish.handler(task, task->finish.p1);
    }else{
        if(task->done_notify != NULL)
        {
            ngx_rpc_notify_push_task(task->done_notify, &(task->node));
        }
    }

    delete channel;
}


