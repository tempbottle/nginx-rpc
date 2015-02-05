#include "ngx_rpc_api.h"

#include<google/protobuf/descriptor.h>
#include<google/protobuf/message.h>
#include<google/protobuf/service.h>


#include "ngx_rpc_router.h"
#include "ngx_rpc_server_impl.h"
#include "ngx_rpc_server_list.h"



ngx_int_t ngx_http_rpc_master_init(ngx_cycle_t *cycle)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "ngx_http_rpc_process_init");

    for(int i=0; all_rpc_services[i]; ++i)
    {
        /*   ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0,
                       "RegisterSerive:%s",
                       all_rpc_services[i]->GetDescriptor()->full_name.c_str());
                       */

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

    //TODO init the TLS data
    return NGX_OK;
}

void ngx_http_rpc_process_exit(ngx_cycle_t *cycle)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "ngx_http_rpc_process_exit");
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

    std::string method((const char *)(r->uri.data), r->uri.len);

    NgxRpcRouter::FindByMethodFullName(method, srv, mdes);

    if(srv == NULL || mdes == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "NOT FOUND:%V", &r->uri);
        ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
        return;
    }

    //7 do with rpc method
    std::shared_ptr< ::google::protobuf::Message>
            req(srv->GetRequestPrototype(mdes).New());
    std::shared_ptr< ::google::protobuf::Message>
            res(srv->GetResponsePrototype(mdes).New());

    // 7 parse the request from the body buffer;


    //std::shared_ptr<NgxRpcServerController>
    //        cntl(new NgxRpcServerController());

    //req->ParseFromArray()

    NgxRpcServerController * cntl = new NgxRpcServerController();
    ::google::protobuf::Closure *done = ::google::protobuf::NewCallback(cntl, &NgxRpcServerController::FinishRequest, req, res);

    ::google::protobuf::Service* ptr = const_cast< ::google::protobuf::Service*>(srv);
    ptr->CallMethod(mdes, cntl, req.get(), res.get(), done);

}
