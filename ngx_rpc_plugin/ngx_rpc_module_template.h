/// includes
static const char * NGX_RPC_MODULE_INLUCDE[] = {
    "",
    "extern \"C\" {",
    "    #include \"ngx_http_rpc.h\"",
    "}",
    "",
    "#include \"$PROTO_NAME$_impl.h\"",
    "#include \"ngx_rpc_channel.h\"",
    "",
    "using namespace $PROTO_NAMESPACE$;"
};

// conf

static const char * NGX_PRC_MODULE_CONF_START[] = {
    "typedef struct",
    "{"
};

static const char * NGX_RPC_MODULE_CONF_SERVERS[] = {
    "    ngx_log_t *$PROTO_SERVER_NAME_LOWER$_log;",
    "    ngx_str_t  $PROTO_SERVER_NAME_LOWER$_conf_file;",
    "    ngx_str_t  $PROTO_SERVER_NAME_LOWER$_log_file;",
    "    $PROTO_SERVER_NAME$_server *$PROTO_SERVER_NAME_LOWER$_impl;"
};

static const char * NGX_PRC_MODULE_CONF_END[] = {
    "} ngx_http_$PROTO_NAME$_conf_t;"
};


static  const char * NGX_RPC_SERVER_SET_CONF[] = {
    "// register the $PROTO_SERVER_NAME$ to ngx_http_rpc_module",
    "static char *ngx_conf_set_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);"
};


///
static const char * NGX_RPC_COMMANDS_START[] = {
    "static ngx_command_t ngx_http_$PROTO_NAME$_commands[] = {"
};


static const char * NGX_RPC_COMMANDS_ITEMS[] = {
    "    ",
    "    { ngx_string(\"ngx_conf_set_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_hanlder\"),",
    "      NGX_HTTP_LOC_CONF | NGX_CONF_ANY,",
    "      ngx_conf_set_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_hanlder,",
    "      NGX_HTTP_LOC_CONF_OFFSET,",
    "      0,",
    "      NULL},",
    "    "
};

static const char * NGX_RPC_COMMANDS_END[] = {
    "     ngx_null_command",
    "};",
};

static const char * NGX_RPC_HTTP_MODULE_DECALRES[] = {
    "static void* ngx_http_$PROTO_NAME$_create_loc_conf(ngx_conf_t *cf);"
};

static const char * NGX_RPC_HTTP_MODULE_DEFINE[] = {

    "static ngx_http_module_t ngx_http_$PROTO_NAME$_module_ctx = {",
    "    NULL,                /* preconfiguration */",
    "    NULL,                /* postconfiguration */",
    "",
    "    NULL,                /* create main configuration */",
    "    NULL,                /* init main configuration */",
    "",
    "    NULL,                /* create server configuration */",
    "    NULL,                /* merge server configuration */",
    "",
    "    ngx_http_$PROTO_NAME$_create_loc_conf,/* create location configuration */",
    "    NULL                 /* merge location configuration */",
    "};"

};


static const char * NGX_RPC_MOUDLE_DECARLES[] = {
    "static void ngx_$PROTO_NAME$_process_exit(ngx_cycle_t* cycle);"
};

static const char * NGX_RPC_MOUDLE_DEFINE[] = {

    "extern \"C\" {",
    "ngx_module_t ngx_http_$PROTO_NAME$_module = {",
    "    NGX_MODULE_V1,",
    "    &ngx_http_$PROTO_NAME$_module_ctx,        /* module context */",
    "    ngx_http_$PROTO_NAME$_commands,          /* module directives */",
    "    NGX_HTTP_MODULE,                         /* module type */",
    "    NULL,                                  /* init master */",
    "    /// there no where called init_master",
    "    /// but some where called init module instead",
    "    NULL,             /* init module */",
    "    NULL,             /* init process */",
    "    NULL,             /* init thread */",
    "    NULL,             /* exit thread */",
    "    ngx_$PROTO_NAME$_process_exit, /* exit process */",
    "    NULL,                     /* exit master */",
    "    NGX_MODULE_V1_PADDING",
    "};",
    "} // for extern C",
};


static const char * NGX_RPC_CREATE_LOC_START[] = {

    "static void* ngx_http_$PROTO_NAME$_create_loc_conf(ngx_conf_t *cf)",
    "{",
    "    ngx_http_$PROTO_NAME$_conf_t *conf = (ngx_http_$PROTO_NAME$_conf_t *)ngx_pcalloc(cf->pool,",
    "                                              sizeof(ngx_http_$PROTO_NAME$_conf_t));",
    ""
};

static const char * NGX_RPC_CREATE_LOC_ITEMS[] = {
    "    conf->$PROTO_SERVER_NAME_LOWER$_conf_file = ngx_string(\"\");",
    "    conf->$PROTO_SERVER_NAME_LOWER$_log_file  = ngx_string(\"\");",
    "    conf->$PROTO_SERVER_NAME_LOWER$_impl      = NULL;"
};

static const char * NGX_RPC_CREATE_LOC_END[] = {
    "    return conf;",
    "}",
};



static const char * NGX_RPC_METHOD_PROCESS[] = {

    "static void ngxrpc_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_$PROTO_SERVER_METHOD_NAME$(ngx_rpc_task_t* _this, void* p1)",
    "{",
    "    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)p1;",
    "",
    "    RpcChannel *cntl = new RpcChannel(rpc_ctx->r);",
    "",
    "    cntl->task = _this;",
    "    cntl->req  = new $PROTO_SERVER_METHOD_REQUEST_NAME$();",
    "    cntl->res  = new $PROTO_SERVER_METHOD_RESPONSE_NAME$();",
    "",
    "    cntl->done =  std::bind(&RpcChannel::finish_request,",
    "                                  std::placeholders::_1,",
    "                                  std::placeholders::_2,",
    "                                  std::placeholders::_3,",
    "                                  std::placeholders::_4);",
    "",
    "",
    "    NgxChainBufferReader reader(_this->req_bufs);",
    "",
    "    if(!cntl->req->ParseFromZeroCopyStream(&reader))",
    "    {",
    "         ngx_log_error(NGX_LOG_ERR, _this->log, 0,",
    "                       \"ParseFromZeroCopyStream req_bufs:%p %d\",",
    "                        _this->req_bufs.buf->pos,",
    "                       (_this->req_bufs.buf->last - _this->req_bufs.buf->pos));",
    "",
    "        cntl->done(cntl, cntl->req, cntl->res, NGX_HTTP_INTERNAL_SERVER_ERROR);",
    "        return;",
    "    }",
    "",
    "    $PROTO_SERVER_NAME$_server * impl = ($PROTO_SERVER_NAME$_server *) rpc_ctx->method->_impl;",
    "",
    "    //call the implement method",
    "    impl->$PROTO_SERVER_METHOD_NAME$(cntl, ($PROTO_SERVER_METHOD_REQUEST_NAME$*)cntl->req, ($PROTO_SERVER_METHOD_RESPONSE_NAME$*)cntl->res, cntl->done);",
    "",
    "}"

};


//for set

static const char * NGX_RPC_SET_SERVER_METHOD_START[] = {

    "static char* ngx_conf_set_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)",
    "{",
    "",
    "    ngx_http_rpc_main_conf_t *rpc_conf = (ngx_http_rpc_main_conf_t *)",
    "            ngx_http_conf_get_module_main_conf(cf, ngx_http_rpc_module);",
    "",
    "    if(rpc_conf == NULL)",
    "    {",
    "        ngx_conf_log_error(NGX_LOG_WARN, cf, 0, \"ngx_http_rpc_module not init\");",
    "        return (char*)\"ngx_http_rpc_module not init\";",
    "    }",
    "",
    "    ngx_http_$PROTO_NAME$_conf_t *$PROTO_NAME$_conf = (ngx_http_$PROTO_NAME$_conf_t*) conf;",
    "    ngx_str_t* value = (ngx_str_t*)cf->args->elts;",
    "",
    "    if(cf->args->nelts > 1)",
    "    {",
    "         $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_conf_file = (value[1]);",
    "    }",
    "",
    "    const std::string app_conf((const char *)$PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_conf_file.data);",
    "    $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_impl = new $PROTO_SERVER_NAME$_server(app_conf);",
    "",
    "",
    "    $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log = ngx_cycle->log;",
    "",
    "    if( cf->args->nelts > 2)",
    "    {",
    "       $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log_file = (value[2]);",
    "       $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log  = (ngx_log_t*)ngx_palloc(cf->pool, sizeof(ngx_log_t));",
    "       $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log->file = ngx_conf_open_file(cf->cycle, &value[1]);",
    "       $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log->log_level = NGX_DEBUG;",
    "    }",
    ""
};

static const char * NGX_RPC_SET_SERVER_METHOD_ITEM[] = {
    "    ",
    "    ",
    "    {",
    "        method_conf_t *method = (method_conf_t *)ngx_array_push(rpc_conf->method_array);",
    "        method->name    = ngx_string(\"/$PROTO_NAMEPATH$/$PROTO_SERVER_NAME$/$PROTO_SERVER_METHOD_NAME$\");",
    "        method->_impl   = $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_impl;",
    "        method->handler = ngxrpc_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_$PROTO_SERVER_METHOD_NAME$;",
    "        method->log     = $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log;",
    "        method->exec_in_nginx = 1;",
    "    }",
    "    ",
    "    {",
    "        method_conf_t *method = (method_conf_t *)ngx_array_push(rpc_conf->method_array);",
    "        method->name    = ngx_string(\"/$PROTO_NAMEPATH$.$PROTO_SERVER_NAME$.$PROTO_SERVER_METHOD_NAME$\");",
    "        method->_impl   = $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_impl;",
    "        method->handler = ngxrpc_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_$PROTO_SERVER_METHOD_NAME$;",
    "        method->log     = $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log;",
    "        method->exec_in_nginx = 1;",
    "    }",
};

static const char * NGX_RPC_SET_SERVER_METHOD_END[] = {
    "",
    "",
    "     ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,",
    "              \"ngx_conf_set_$PROTO_NAME$_$PROTO_SERVER_NAME_LOWER$_hanlder with $PROTO_SERVER_NAME_LOWER$_impl:%p, conf file:%V log_file:%V\",",
    "              $PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_impl,",
    "              &$PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_conf_file,",
    "              &$PROTO_NAME$_conf->$PROTO_SERVER_NAME_LOWER$_log_file);",
    "",
    "",
    "     return NGX_OK;",
    "}"
};




//for exit
static const char * NGX_RPC_EXIT_START [] = {

    "static void ngx_$PROTO_NAME$_process_exit(ngx_cycle_t* cycle)",
    "{",
    "    ngx_http_conf_ctx_t * ctx = (ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index];",
    "    ngx_http_$PROTO_NAME$_conf_t *c = (ngx_http_$PROTO_NAME$_conf_t *) ctx ->loc_conf[ngx_http_module.ctx_index];",
    "",
    "    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, \"ngx_$PROTO_NAME$_process_exit done\");",
    ""
};

static const char * NGX_RPC_EXIT_ITEM[] = {
    "    // release the instance ",
    "    delete c->$PROTO_SERVER_NAME_LOWER$_impl;",
    ""
};

static const char * NGX_RPC_EXIT_END[] = {
    "",
    "}"
};






/// for config
static const char * NGX_RPC_CONFIG[] = {
"#Add this module when configure nginx with --add-module=\"ngx_rpc_%PROTO_NAME%_module\"",
"#this is the module name",
"ngx_addon_name=\"ngx_http_%PROTO_NAME%_module\"",
"",
"HTTP_MODULES=\"$HTTP_MODULES ngx_http_%PROTO_NAME%_module \"",
"",
"#sources files with lowwer case",
"NGX_ADDON_SRCS=\"$NGX_ADDON_SRCS \\",
"    $ngx_addon_dir/%PROTO_NAME%.pb.cc \\",
"    $ngx_addon_dir/ngx_http_%PROTO_NAME%_module.cpp \"",
"",
"#this header files",
"NGX_ADDON_DEPS=\"$NGX_ADDON_DEPS \\",
"    $ngx_addon_dir/%PROTO_NAME%_impl.h \\",
"    $ngx_addon_dir/%PROTO_NAME%.pb.h \"",
"",
"CORE_INCS=\"$CORE_INCS $ngx_addon_dir\"",
""
};


