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

         response->set_json("123456");
         //finish this method
         done(channel,request,response,NGX_HTTP_OK);
    }



     void requeststatus(RpcChannel *channel,const Request* request,
                                     Response* response, RpcCallHandler done)
    {
         DEBUG("requeststatus was called!");

         RpcCallHandler updone_done = std::bind(&Application_server::requeststatus_forward_done, this,
                                                 channel, request, response,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3,
                                                 std::placeholders::_4);

         Request*  up_req = new Request();
         Response* up_res = new Response();

         up_req->CopyFrom(*request);

         channel->upstream("/ngxrpc/inspect/Application/interface", up_req, up_res, updone_done);
    }

     void requeststatus_forward_done(RpcChannel *  orgin_channel,
                                     const  ::ngxrpc::inspect::Request * orgin_request,
                                     ::ngxrpc::inspect::Response* orgin_response,

                                     RpcChannel *new_channel,
                                     const ::google::protobuf::Message* new_request,
                                     ::google::protobuf::Message* new_response,
                                     int new_status)
     {

         ::ngxrpc::inspect::Response * sub_response =  (::ngxrpc::inspect::Response *) new_response;
         orgin_response->set_json(sub_response->json());

         DEBUG("requeststatus_forward_done orgin_response:"<<orgin_response->json());
         orgin_channel->done(orgin_channel, new_request, new_response, new_status);
     }


}; /// for class Application_server



} // for ngxrpc 
} //for inspect



#endif // for __INSPECT_SERVER_IMPL_H_



