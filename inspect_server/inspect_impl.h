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
                    RpcCallHandler done);

     void requeststatus(RpcChannel *channel,
                     const ::ngxrpc::inspect::Request* request,
                     ::ngxrpc::inspect::Response* response,
                     RpcCallHandler done);

    void requeststatus_forward_done(RpcChannel *orgin_channel,
                                    const  ::ngxrpc::inspect::Request * orgin_request,
                                    ::ngxrpc::inspect::Response* orgin_response,

                                    RpcChannel *new_channel,
                                    const ::google::protobuf::Message* new_request,
                                    ::google::protobuf::Message* new_response,
                                    int new_status);
private:
};


}} // for namespace ngxrpc { namespace inspect {


#endif // INSPECT_SERVER_IMPL_H
