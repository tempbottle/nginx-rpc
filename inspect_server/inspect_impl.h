#ifndef INSPECT_SERVER_IMPL_H
#define INSPECT_SERVER_IMPL_H

#include "inspect.pb.h"
#include "ngx_rpc_channel.h"


/// simple is better

namespace ngxrpc { namespace inspect {


/// a closure
class ApplicationServer
{
public:


     void interface(RpcChannel *channel,
                    const ::ngxrpc::inspect::Request* request,
                    ::ngxrpc::inspect::Response* response,
                    RpcCallHandler done)
     {
         response->set_json("hehe");
         // TODO your process
         done(channel, request, response, NGX_HTTP_OK);
     }

     void requeststatus(RpcChannel *channel,
                     const ::ngxrpc::inspect::Request* request,
                     ::ngxrpc::inspect::Response* response,
                     RpcCallHandler done)
     {
         // TODO your process
         done(channel, request, response, NGX_OK);
     }

private:


};


class ApplicationClient {
public:
    void interface(RpcChannel *channel,const ::ngxrpc::inspect::Request* request,
                   ::ngxrpc::inspect::Response* response,
                   RpcCallHandler done )
    {

        channel->start_subrequest("/ngxrpc/inspect/Application/interface",request, response, done);
    }


    void requeststatus(RpcChannel *channel,
                       const ::ngxrpc::inspect::Request* request,
                       ::ngxrpc::inspect::Response* response,
                       RpcCallHandler done)
    {
        channel->start_subrequest("/ngxrpc/inspect/Application/interface",request, response, done);
    }
public:
};

}}






#endif // INSPECT_SERVER_IMPL_H
