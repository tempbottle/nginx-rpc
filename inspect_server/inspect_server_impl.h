#ifndef INSPECT_SERVER_IMPL_H
#define INSPECT_SERVER_IMPL_H

#include "inspect.pb.h"
#include "ngx_rpc_module/ngx_rpc_server_impl.h"

///
/// \brief The AppStaticData class
///
///  static object which construct before the main
///  should not modify after init



class ApplicationServiceImpl : public NgxRpcServer<mtrpc::inspect::Application>
{
public:

    virtual void interface(::google::protobuf::RpcController* controller,
                         const ::mtrpc::inspect::Request* request,
                         ::mtrpc::inspect::Response* response,
                         ::google::protobuf::Closure* done);

};



#endif // INSPECT_SERVER_IMPL_H
