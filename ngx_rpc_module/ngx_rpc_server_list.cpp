#include "ngx_rpc_server_list.h"

#include"inspect_server/inspect_server_impl.h"



// all service
::google::protobuf::Service* all_rpc_services[]= {
    new ApplicationServiceImpl(),
    NULL,
};


