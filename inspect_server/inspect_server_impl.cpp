#include "inspect_server_impl.h"



void ApplicationServiceImpl::interface(
        google::protobuf::RpcController *controller,
        const mtrpc::inspect::Request *request,
        mtrpc::inspect::Response *response,
        google::protobuf::Closure *done){

ReadonlyClass * read = GetReadonlyClass();

TlsClass* tls = GetTls();

    // do some thing
    //must do
    done->Run();
}

