#include "inspect_server_impl.h"



void ApplicationServiceImpl::interface(
        google::protobuf::RpcController *controller,
        const ngxrpc::inspect::Request *request,
        ngxrpc::inspect::Response *response,
        google::protobuf::Closure *done){

   GetReadonly();

    // do some thing
    //must do
    done->Run();
}

