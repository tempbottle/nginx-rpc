/// defines
static const char * NGX_RPC_SERVER_HEADER_START[] = {
    "#ifndef __$PROTO_NAME_UPPER$_SERVER_IMPL_H__",
    "#define __$PROTO_NAME_UPPER$_SERVER_IMPL_H__"
};

static const char * NGX_RPC_SERVER_HEADER_END[] = {
    "#endif // for __$PROTO_NAME_UPPER$_SERVER_IMPL_H_"
};


static const char * NGX_RPC_SERVER_INCLUDE[]= {
    "#include \"$PROTO_NAME$.pb.h\"",
    "#include \"ngx_rpc_channel.h\""
};



/// name space

static const char* NGX_RPC_SERVER_NAMESPACE_START[] =
{
    "namespace $PROTO_NAMESPACE$ {"
};

static const char* NGX_RPC_SERVER_NAMESPACE_END[] =
{
    "} // for $PROTO_NAMESPACE$"
};




static const char* NGX_RPC_SERVER_CLASS_START[] = {
    "class $PROTO_SERVER_NAME$_server ",
    "{",
    "public:",
    "    $PROTO_SERVER_NAME$_server(const std::string& conf_file)",
    "    {",
    "    }",
};

static const char* NGX_RPC_SERVER_CLASS_END[] = {
    "}; /// for class $PROTO_SERVER_NAME$_server"
};


static const char* NGX_RPC_SERVER_METHOD[]={

    "     void $PROTO_SERVER_METHOD_NAME$(RpcChannel *channel,const $PROTO_SERVER_METHOD_REQUEST_NAME$* request,",
    "                                     $PROTO_SERVER_METHOD_RESPONSE_NAME$* response, RpcCallHandler done)",
    "    {",
    "         DEBUG(\"$PROTO_SERVER_METHOD_NAME$ was called!\")",
    "",
    "         //finish this method",
    "         done(channel,request,response,NGX_HTTP_OK);",
    "    }"
};


