#include "inspect_impl.h"

using namespace ngxrpc::inspect;



void ApplicationServer::interface(RpcChannel *channel,
                                  const ::ngxrpc::inspect::Request* request,
                                  ::ngxrpc::inspect::Response* response,
                                  RpcCallHandler done)
{
    response->set_json("hehe");
    // TODO your process

    DEBUG("interface response:"<<response->json());

    done(channel, request, response, NGX_HTTP_OK);
}

void ApplicationServer::requeststatus(RpcChannel *channel,
                                      const ::ngxrpc::inspect::Request* request,
                                      ::ngxrpc::inspect::Response* response,
                                      RpcCallHandler done)
{

    // TODO your process
    channel->done = done;

    RpcCallHandler forward_done = std::bind(&ApplicationServer::requeststatus_forward_done, this,
                                            channel, request, response,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3,
                                            std::placeholders::_4);



    ::ngxrpc::inspect::Request* new_request = request->New();
    new_request->CopyFrom(*request);

    ::ngxrpc::inspect::Response* new_respone = response->New();

    channel->forward_request("/ngxrpc/inspect/application/interface", new_request, new_respone, forward_done);
}

void ApplicationServer::requeststatus_forward_done(RpcChannel *orgin_channel,
                                                   const  ::ngxrpc::inspect::Request * orgin_request,
                                                   ::ngxrpc::inspect::Response* orgin_response,

                                                   RpcChannel *new_channel,
                                                   const ::google::protobuf::Message* new_request,
                                                   ::google::protobuf::Message* new_response,
                                                   int new_status)
{
    // do process

    ::ngxrpc::inspect::Response * sub_response =  (::ngxrpc::inspect::Response *) new_response;
    orgin_response->set_json(sub_response->json());

    DEBUG("requeststatus_forward_done orgin_response:"<<orgin_response->json());

    orgin_channel->done(orgin_channel, new_request, new_response, new_status);


}