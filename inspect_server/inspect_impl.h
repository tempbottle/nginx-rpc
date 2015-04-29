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

         RpcCallHandler forward_done = std::bind(&ApplicationServer::requeststatus_forward_done, this,
                                                 channel, request, response,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3,
                                                 std::placeholders::_4);



         ::ngxrpc::inspect::Request* new_request = request->New();
          new_request->CopyFrom(*request);

         ::ngxrpc::inspect::Response* new_respone = response->New();

         channel->forward_request("/ngxrpc/inspect/application/requeststatus", new_request, new_respone, forward_done);
    }

    void requeststatus_forward_done(RpcChannel *orgin_channel,
                                    const  ::google::protobuf::Message* orgin_request,
                                    ::ngxrpc::inspect::Response* orgin_response,

                                    RpcChannel *new_channel,
                                    const ::google::protobuf::Message* new_request,
                                    ::google::protobuf::Message* new_response,
                                    int new_status)
    {
        // do process
        orgin_channel->done(new_channel, new_request, new_response, new_status);
    }

private:


};


class ApplicationClient {
public:
    void interface(RpcChannel *channel,const ::ngxrpc::inspect::Request* request,
                   ::ngxrpc::inspect::Response* response,
                   RpcCallHandler done )
    {

        channel->forward_request("/ngxrpc/inspect/Application/interface",request, response, done);
    }


    void requeststatus(RpcChannel *channel,
                       const ::ngxrpc::inspect::Request* request,
                       ::ngxrpc::inspect::Response* response,
                       RpcCallHandler done)
    {
        channel->forward_request("/ngxrpc/inspect/Application/interface",request, response, done);
    }
public:
};

}}






#endif // INSPECT_SERVER_IMPL_H
