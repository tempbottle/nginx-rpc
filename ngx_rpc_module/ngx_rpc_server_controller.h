#ifndef _NGX_RPC_SERVER_CONTROLER_H_
#define _NGX_RPC_SERVER_CONTROLER_H_

#include <memory>
#include "google/protobuf/service.h"
#include "google/protobuf/message.h"

#include "ngx_rpc_api.h"


class NgxRpcServerController :
        public ::google::protobuf::RpcController {
public:
    NgxRpcServerController(
            ngx_http_request_t* request,
            ::google::protobuf::Message* req_msg,
            ::google::protobuf::Message* res_msg):
        req(req_msg),
        res(res_msg),
        r(request)
    {
    }


    static void FinishRequest(
             std::shared_ptr<  NgxRpcServerController> cntl);
public:
    std::shared_ptr< ::google::protobuf::Message> req;
    std::shared_ptr< ::google::protobuf::Message> res;



public:


    virtual void Reset()
    {
    }

    virtual bool Failed() const
    {
        return true;
    }

    virtual std::string ErrorText() const
    {

        return "";
    }

    virtual void StartCancel()
    {
    }

    virtual void SetFailed(const std::string& reason)
    {

    }

    virtual bool IsCanceled() const
    {
        return false;
    }

    virtual void NotifyOnCancel(
            ::google::protobuf::Closure* callback)
    {

    }

public:


public:
    ngx_http_request_t* r;

};



#endif
