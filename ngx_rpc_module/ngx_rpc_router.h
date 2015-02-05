#ifndef NGX_RPC_ROUTER_H
#define NGX_RPC_ROUTER_H
#include <google/protobuf/service.h>

#include<unordered_map>

#include <string>


class NgxRpcRouter
{
    public:
    typedef std::pair<const ::google::protobuf::Service*, const ::google::protobuf::MethodDescriptor*> ValueType;

     static void RegisterSerive(const ::google::protobuf::Service* ser);

     static void FindByMethodFullName(const std::string & name,
                       const ::google::protobuf::Service* &srv,
                       const google::protobuf::MethodDescriptor* &mdes);

public:
     static std::unordered_map<std::string, ValueType> route_table;
};




#endif // NGX_RPC_ROUTER_H
