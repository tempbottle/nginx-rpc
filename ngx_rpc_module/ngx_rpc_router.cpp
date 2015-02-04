#ifndef NGX_RPC_ROUTER_CPP
#define NGX_RPC_ROUTER_CPP

#include "ngx_rpc_router.h"

//static member
std::unorder_map<std::string, NgxRpcRouter::ValueType>
                  NgxRpcRouter::route_table;


void NgxRpcRouter::RegisterSerive(const ::google::protobuf::Service *ser)
{
    const ::google::protobuf::ServiceDescriptor *sdes = ser->GetDescriptor();


    for(int i=0; i< sdes->method_count(); i++)
    {
        const::google::protobuf::MethodDescriptor *mdes = sdes->method(i);

        const std::string& full_name = "/" + mdes->full_name();

        route_table[full_name] = std::make_pair<ser,mdes>;
    };

}

void NgxRpcRouter::FindByMethodFullName(const std::string & name,
       const ::google::protobuf::Service* &srv,
       const ::google::protobuf::MethodDescriptor *mdes)
{
      auto it = route_table.find(name);

      if(it != route_table.end())
      {
          srv = it.second.first;
          mdes = it.second.second;
      }else{
          srv = nullptr;
          mdes = nullptr;
      }
}





#endif // NGX_RPC_ROUTER_CPP
