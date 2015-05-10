#include <google/protobuf/compiler/plugin.h>
#include "ngx_rpc_generator.h"


int main(int argc, char* argv[]) {
    NginxRpcGenerator generator;
     return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}


