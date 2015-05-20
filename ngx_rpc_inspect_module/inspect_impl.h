#ifndef __INSPECT_SERVER_IMPL_H__
#define __INSPECT_SERVER_IMPL_H__



#include "inspect.pb.h"
#include "ngx_rpc_channel.h"



namespace ngxrpc {
namespace inspect {



class Application_server 
{
public:
    Application_server(const std::string& conf_file)
    {
    }



     void interface(RpcChannel *channel,const Request* request,
                                     Response* response, RpcCallHandler done)
    {
         DEBUG("interface was called!");

         //finish this method
         done(channel,request,response,NGX_HTTP_OK);
    }



     void requeststatus(RpcChannel *channel,const Request* request,
                                     Response* response, RpcCallHandler done)
    {
         DEBUG("requeststatus was called!");

         //finish this method
         done(channel,request,response,NGX_HTTP_OK);
    }



}; /// for class Application_server



} // for ngxrpc 
} //for inspect



#endif // for __INSPECT_SERVER_IMPL_H_



