#include "inspect_application_impl.h"



//for each sever  eache method 
void ApplicationServiceImpl::interface(google::protobuf::RpcController *controller,
                                      const ngxrpc::inspect::Request *request,
                                      ngxrpc::inspect::Response *response,
                                      google::protobuf::Closure *done)
{

    TRACE("interface");
    done->Run();
    
}

//for each sever  eache method 
void ApplicationServiceImpl::requeststatus(google::protobuf::RpcController *controller,
                                      const ngxrpc::inspect::Request *request,
                                      ngxrpc::inspect::Response *response,
                                      google::protobuf::Closure *done)
{

    TRACE("interface");
    done->Run();
    
}

