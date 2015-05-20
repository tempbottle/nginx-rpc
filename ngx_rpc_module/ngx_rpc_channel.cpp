
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

    rpc_call_channel->req  = (::google::protobuf::Message*) req;
    rpc_call_channel->res  = res;
    rpc_call_channel->done = done;

    // only one upstream can do in same time ,so reuse this task
    rpc_call_channel->task = task;

    ngx_http_rpc_task_set_bufs(task, &task->req_bufs, NULL);
    task->req_length = 0;

    ngx_http_rpc_task_set_bufs(task, &task->res_bufs, NULL);
    task->res_length = 0;


    if(task->exec_in_nginx)
    {
        NgxChainBufferWriter  writer(rpc_call_channel->task->req_bufs, rpc_call_channel->task->ngx_pool);
        req->SerializeToZeroCopyStream(&writer);
        rpc_call_channel->task->req_length = writer.totaly;
    }else{
        NgxShmChainBufferWriter  writer(rpc_call_channel->task->req_bufs, rpc_call_channel->task->share_pool);
        req->SerializeToZeroCopyStream(&writer);
        rpc_call_channel->task->req_length = writer.totaly;
    }

    rpc_call_channel->task->finish.handler = ngx_http_rpc_start_upsteam;
    rpc_call_channel->task->finish.p1      = this->r;

    rpc_call_channel->task->closure.handler = RpcChannel::upstream_done;
    rpc_call_channel->task->closure.p1      = rpc_call_channel;

    strncpy((char*)rpc_call_channel->task->path, upstream_path.c_str(), upstream_path.size());

    if(rpc_call_channel->task->exec_in_nginx)
    {
        rpc_call_channel->task->finish.handler(rpc_call_channel->task,
                                               rpc_call_channel->task->finish.p1);
    }else{
        ngx_rpc_notify_push_task(rpc_call_channel->task->done_notify,
                                 &rpc_call_channel->task->node);
    }
}

 void RpcChannel::upstream_done(ngx_rpc_task_t* task, void *ctx)
{

    RpcChannel *new_channel = (RpcChannel *)ctx;
    new_channel->task->exec_upstream = 0;

    NgxChainBufferReader reader(new_channel->task->res_bufs);
    bool ret = false;

    if(new_channel->task->response_states == NGX_HTTP_OK)
    {
        ret = new_channel->res->ParseFromZeroCopyStream(&reader);
    }

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                  " upstream_done task:%p, status:%d length:%d parse:%d",
                  new_channel->task, new_channel->task->response_states,
                  new_channel->task->res_length, ret);

    new_channel->done(new_channel->pre_cntl, new_channel->req,
                      new_channel->res, new_channel->task->response_states);

    delete new_channel;

}


void RpcChannel::subrequest(const std::string& path,
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

    new_channel->task = task;

    ngx_http_rpc_task_set_bufs(task, &task->req_bufs, NULL);
    task->req_length = 0;

    ngx_http_rpc_task_set_bufs(task, &task->res_bufs, NULL);
    task->res_length = 0;

    bool ret = false;
    if(new_channel->task->exec_in_nginx)
    {
        NgxChainBufferWriter  writer(new_channel->task->req_bufs, new_channel->task->ngx_pool);
        ret = req->SerializeToZeroCopyStream(&writer);
        new_channel->task->req_length = writer.totaly;

    }else{

        NgxShmChainBufferWriter  writer(new_channel->task->req_bufs, new_channel->task->share_pool);
        ret = req->SerializeToZeroCopyStream(&writer);
        new_channel->task->req_length = writer.totaly;
    }

    // reuse this task
    new_channel->task->finish.handler = ngx_http_rpc_request_foward;
    new_channel->task->finish.p1 = r;

    new_channel->task->closure.handler = RpcChannel::subrequest_done;
    new_channel->task->closure.p1      = new_channel;

    strncpy((char*)new_channel->task->path, path.c_str(), path.size());

    if(new_channel->task->done_notify != NULL)
         ngx_rpc_notify_push_task(new_channel->task->done_notify,
                                  &new_channel->task->node);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                  "forward_request SerializeToZeroCopyStream:%d res_length:%d nofity:%p path:%s",
                  ret, new_channel->task->res_length, new_channel->task->done_notify, new_channel->task->path);

}


void RpcChannel::subrequest_done(ngx_rpc_task_t* task, void *ctx)
{
    task->exec_subrequest = 0;

    RpcChannel *new_channel = (RpcChannel *)ctx;

    NgxChainBufferReader reader(task->res_bufs);
    bool ret = false;


    if(new_channel->task->response_states == NGX_HTTP_OK)
    {
        ret = new_channel->res->ParseFromZeroCopyStream(&reader);
    }

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, " subrequest_done task:%p, status:%d length:%d parse:%d", new_channel->task, new_channel->task->response_states, new_channel->task->res_length, ret);

    new_channel->done(new_channel->pre_cntl, new_channel->req,
                      new_channel->res, task->response_states);

    delete new_channel;
}



void RpcChannel::finish_request(RpcChannel *channel, const google::protobuf::Message *req, google::protobuf::Message *res, int result)
{
    ngx_rpc_task_t *task = channel->task;
    task->response_states = result;

    if(task->response_states == NGX_HTTP_OK)
    {
        bool ret = false;

        ngx_http_rpc_task_set_bufs(task, &task->res_bufs, NULL);
        task->res_length = 0;

        if(task->exec_in_nginx)
        {
            NgxChainBufferWriter writer(task->res_bufs, task->ngx_pool);
            ret = res->SerializeToZeroCopyStream(&writer);
            task->res_length = writer.totaly;

        }else{

            NgxShmChainBufferWriter  writer(task->res_bufs, task->share_pool);
            ret = res->SerializeToZeroCopyStream(&writer);
            task->res_length = writer.totaly;
        }

        // content-length shoul not be 0
        if(task->res_length <= 0)
        {
            task->response_states = NGX_HTTP_INTERNAL_SERVER_ERROR;

            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                          "Reponse can not be empty, SerializeToZeroCopyStream:%d size:%d notify eventfd:%d",
                          ret, task->res_length, task->done_notify->event_fd);
        }else{

            ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                          "SerializeToZeroCopyStream:%d size:%d notify eventfd:%d",
                          ret, task->res_length, task->done_notify->event_fd);
        }
    }else{

        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                      "finish_request task:%p notify eventfd:%d response_states",
                      task, task->done_notify->event_fd, task->response_states);
    }

    task->finish.handler = ngx_http_rpc_request_finish;
    task->finish.p1 = channel->r;

    if(task->exec_in_nginx)
    {
        task->finish.handler(task, task->finish.p1);
    }else{
        if(task->done_notify != NULL)
        {
            ngx_rpc_notify_push_task(task->done_notify, &(task->node));
        }
    }

    delete channel->req;
    delete channel->res;
    delete channel;
}


