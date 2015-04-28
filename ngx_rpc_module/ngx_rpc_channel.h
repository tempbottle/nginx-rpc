#ifndef __NGX_RPC_CHANNEL_H_
#define __NGX_RPC_CHANNEL_H_

extern "C" {
   #include "ngx_rpc_task.h"
}


#include <memory>
#include <functional>



#include "google/protobuf/message.h"

#include "ngx_log_cpp.h"
#include "ngx_rpc_buffer.h"




class RpcChannel;

typedef std::function <void(RpcChannel *channel,
                            const ::google::protobuf::Message*,
                            ::google::protobuf::Message*,
                            int)> RpcCallHandler;


class RpcChannel{
public:
    RpcChannel(ngx_http_request_t* _r)
        :r(_r),req(NULL),res(NULL)
    {
    }

    static void destructor(void *p);

    int setRequestHeader();

    int getRequestHeader();

    int setResponseHeader();

    int getResponseHeader();

    void forward_request(const std::string& path,
                     const ::google::protobuf::Message* req,
                     ::google::protobuf::Message* res,
                     RpcCallHandler done);



    static void foward_done(ngx_rpc_task_t* _this, void *ctx);
    static void finish_request(RpcChannel *channel,
                               const ::google::protobuf::Message* req,
                               ::google::protobuf::Message* res,
                               int result);


public:
    ngx_http_request_t* r;
    ngx_rpc_task_t *task;

    ::google::protobuf::Message *req;
    ::google::protobuf::Message *res;

    RpcChannel* pre_cntl;
    RpcCallHandler  done;
};




#endif

