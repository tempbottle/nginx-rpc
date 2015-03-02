#include <sys/eventfd.h>


#include "ngx_rpc_api.h"
#include "ngx_log_cpp.h"

#include "ngx_rpc_router.h"
#include "ngx_rpc_server_impl.h"
#include "ngx_rpc_server_list.h"
#include "ngx_rpc_buffer.h"
#include "ngx_rpc_server_controller.h"


#include<google/protobuf/descriptor.h>
#include<google/protobuf/message.h>
#include<google/protobuf/service.h>

#define MAX_NOTIFY_TASK_MAX 1024

#include <vector>


// notify the proccess cycle do something
static ngx_connection_t* notify_conn = NULL;
static std::vector<ngx_notify_queue_t> notify_task;
ngx_notify_queue_t ngx_rpc_post_start;


int ngx_rpc_post_task(ngx_rpc_post_handler_pt callback, void *data)
{
    if(notify_task.size() >= MAX_NOTIFY_TASK_MAX)
    {
        return NGX_ERROR;
    }

    ngx_notify_queue_t task = {callback, data};
    notify_task.push_back(task);

    ngx_int_t signal = 1;

    ::write(notify_conn->fd, (char*)&signal, sizeof(signal));

     return NGX_OK;
}

static void ngx_event_hanlder_notify_write(ngx_event_t *ev)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0, "ngx_event_hanlder_notify_write");

    //do nothing,this should not called;

    if(ngx_rpc_post_start.handler)
    {
        ngx_rpc_post_start.handler(ngx_rpc_post_start.ctx);
    }

    ngx_rpc_post_start.handler = NULL;
    ngx_rpc_post_start.ctx  = NULL;

    ngx_del_event(ev, NGX_WRITE_EVENT, 0);
}


static void ngx_event_hanlder_notify_read(ngx_event_t *ev)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ev->log, 0,"ngx_event_hanlder_notify_read");

    //read and do all the work;
     ngx_int_t signal = 1;

     int ret = 0;
     do{
         ret = ::read(notify_conn->fd, (char*)&signal, sizeof(signal));
     }while(ret > 0);

     while(!notify_task.empty())
     {
          ngx_notify_queue_t task = notify_task.back();
          notify_task.pop_back();
          task.handler(task.ctx);
     }

     //ngx_del_event(ev, NGX_READ_EVENT, 0);
}



ngx_int_t ngx_http_rpc_master_init(ngx_cycle_t *cycle)
{
    set_cache_log(cycle->log);

    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "ngx_http_rpc_process_init log done");

    for(int i=0; all_rpc_services[i]; ++i)
    {
        INFO("RegisterSerive:"<<all_rpc_services[i]->GetDescriptor()->full_name());
        NgxRpcRouter::RegisterSerive(all_rpc_services[i]);
    }

    return NGX_OK;
}

void ngx_http_rpc_master_exit(ngx_cycle_t *cycle)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "ngx_http_rpc_process_exit");

    return;
}

ngx_int_t ngx_http_rpc_process_init(ngx_cycle_t *cycle)
{
    // do some init here
    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "ngx_http_rpc_process_init");

    int event_fd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
    notify_conn = ngx_get_connection(event_fd, cycle->log);

    notify_conn->pool = cycle->pool;

    notify_conn->read->handler = ngx_event_hanlder_notify_read;
    notify_conn->read->log = cycle->log;


    notify_conn->write->handler = ngx_event_hanlder_notify_write;
    notify_conn->write->log = cycle->log;

    if( ngx_add_conn(notify_conn) != NGX_OK)
    {
        return NGX_ERROR;
    }

    //TODO init the TLS data
    return NGX_OK;
}

void ngx_http_rpc_process_exit(ngx_cycle_t *cycle)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "ngx_http_rpc_process_exit");

    ::close(notify_conn->fd);
    ngx_del_conn(notify_conn, 0);
    ngx_free_connection(notify_conn);

    return;
}

void ngx_http_rpc_post_handler(ngx_http_request_t *r)
{

    // 1 init the ctx
    ngx_http_rpc_ctx *ctx =
            ( ngx_http_rpc_ctx *)ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    if(ctx == NULL)
    {
        ctx = (ngx_http_rpc_ctx *)ngx_palloc(r->pool, sizeof(ngx_http_rpc_ctx));
        if(ctx == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "ngx_palloc error size:%d", sizeof(ngx_http_rpc_ctx));
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
        //TODO init the ctx here
        ngx_http_set_ctx(r, ctx, ngx_http_rpc_module);
    }

    // 2 get the content
    // ngx_http_rpc_conf *conf = ngx_http_get_module_loc_conf(r, ngx_http_rpc_module);



    //3  check the request
    if(r->request_body == NULL
            || r->request_body->bufs == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Request body is NULL");
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }

    //4 do with the content-type


    //5  find the route serices
    const ::google::protobuf::Service* srv = NULL;
    const ::google::protobuf::MethodDescriptor *mdes = NULL;
    const std::string method((const char *)(r->uri.data), r->uri.len);

    NgxRpcRouter::FindByMethodFullName(method, srv, mdes);

    if(srv == NULL || mdes == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "NOT FOUND:%V", &r->uri);
        ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
        return;
    }

    //7 do with rpc method

    std::shared_ptr<NgxRpcServerController> cntl(
                new NgxRpcServerController(
                        r,
                        srv->GetRequestPrototype(mdes).New(),
                        srv->GetResponsePrototype(mdes).New()
                    )
                );

    NgxChainBufferReader reader(*r->request_body->bufs);

    if(!cntl->req->ParseFromZeroCopyStream(&reader))
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log,
                      0,
                      "parser request fialed:%V, msg name:%s",
                      &r->uri, cntl->req->GetTypeName().c_str());

        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }


    ::google::protobuf::Closure *done =
            ::google::protobuf::NewCallback(
                &NgxRpcServerController::FinishRequest, cntl);

    ::google::protobuf::Service* ptr =
            const_cast< ::google::protobuf::Service*>(srv);

     ptr->CallMethod(mdes, cntl.get(), cntl->req.get(), cntl->res.get(), done);

}
