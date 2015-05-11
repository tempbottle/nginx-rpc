/// includes
static const char * NGX_RPC_MODULE_INLUCDE[] = {
    "extern \"C\" {",
    " #include \"ngx_http_rpc.h\"",
    "}",
    " #include \"${PROTO_NAME}_impl.h\"",
    "#include \"ngx_rpc_channel\"",
    "using namespace ${PROTO_NAMESPACE};"
};



// conf

static const char * NGX_PRC_MODULE_CONF_START[] = {
    "typedef struct",
    "{"
};

static const char * NGX_RPC_MODULE_CONF_SERVERS[] = {
    "ngx_str_t  ${PROTO_SERVER_NAME_LOWER}_conf_file;",
    "ngx_str_t  ${PROTO_SERVER_NAME_LOWER}_log_file;",
    "${PROTO_SERVER_NAME}Server * ${PROTO_SERVER_NAME_LOWER}_impl;",
    "ngx_log_t*         ${PROTO_SERVER_NAME_LOWER}_log;"

};

static const char * NGX_PRC_MODULE_CONF_END[] = {
    " } ngx_http_${PROTO_NAME}_conf_t;"
};


static  const char * NGX_RPC_SERVER_SET_CONF[] =  {
    "static char *ngx_conf_set_${PROTO_NAME}_${PROTO_SERVER_NAME_LOWER}_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);"
};


///
static const char * NGX_RPC_COMMANDS_START[] = {
    "static ngx_command_t ngx_http_${PROTO_NAME}_commands[] = {"
};


static const char * NGX_RPC_COMMANDS_ITEMS[] = {
    "    ",
    "{ ngx_string(\"ngx_conf_set_${PROTO_NAME}_${PROTO_SERVER_NAME_LOWER}_hanlder\"),",
    "NGX_HTTP_LOC_CONF | NGX_CONF_ANY,",
    "ngx_conf_set_${PROTO_NAME}_${PROTO_SERVER_NAME_LOWER}_hanlder,",
    "NGX_HTTP_LOC_CONF_OFFSET,",
    "0,",
    "NULL },"
};

static const char * NGX_RPC_COMMANDS_END[] = {
    "ngx_null_command",
    "};",
};

static const char * NGX_RPC_HTTP_MODULE_DECALRES[] = {
    "static void* ngx_http_${PROTO_NAME}_create_loc_conf(ngx_conf_t *cf);"
};

static const char * NGX_RPC_HTTP_MODULE_DEFINE[] = {

    "static ngx_http_module_t ngx_http_${PROTO_NAME}_module_ctx = {",
    "    NULL,                /* preconfiguration */",
    "    NULL,                /* postconfiguration */",
    "",
    "    NULL,                /* create main configuration */",
    "    NULL,                /* init main configuration */",
    "",
    "    NULL,                /* create server configuration */",
    "   NULL,                /* merge server configuration */",
    "",
    "   ngx_http_${PROTO_NAME}_create_loc_conf,",
    "   /* create location configuration */",
    "    NULL                 /* merge location configuration */",
    ",};"

};


static const char * NGX_RPC_MOUDLE_DECARLES[] = {
    "static void ngx_${PROTO_NAME}_process_exit(ngx_cycle_t* cycle);"
};

static const char * NGX_RPC_MOUDLE_DEFINE[] = {

    "extern \"C\" {",
    "ngx_module_t ngx_http_${PROTO_NAME}_module = {",
    "   NGX_MODULE_V1,",
    "   &ngx_http_${PROTO_NAME}_module_ctx,        /* module context */",
    "    ngx_http_${PROTO_NAME}_commands,          /* module directives */",
    "    NGX_HTTP_MODULE,                         /* module type */",
    "    NULL,                                  /* init master */",
    "    /// there no where called init_master",
    "   /// but some where called init module instead",
    "    NULL,             /* init module */",
    "    NULL,             /* init process */",
    "   NULL,             /* init thread */",
    "    NULL,             /* exit thread */",
    "   ngx_inspect_process_exit, /* exit process */",
    "   NULL,                     /* exit master */",
    "   NGX_MODULE_V1_PADDING",
    "};",

    "}",
};


static const char * NGX_RPC_CREATE_LOC_START[] = {

    "    static void* ngx_http_${PROTO_NAME}_create_loc_conf(ngx_conf_t *cf),",
    "   {,",
    "       ngx_http_${PROTO_NAME}_conf_t *conf = (ngx_http_${PROTO_NAME}_conf_t *),",
    "               ngx_pcalloc(cf->pool, sizeof(ngx_http_${PROTO_NAME}_conf_t));"
};

static const char * NGX_RPC_CREATE_LOC_ITEMS[] = {
    "    conf->${PROTO_SERVER_NAME_LOWER}_conf_file = ngx_string("");",
    "    conf->${PROTO_SERVER_NAME_LOWER}_log_file = ngx_string("");"
};

static const char * NGX_RPC_CREATE_LOC_END[] = {
    "     return conf;",
    " }",
};



static const char * NGX_RPC_METHOD_PROCESS[] = {

    "  static void ngxrpc_${PROTO_NAME}_${PROTO_SERVER_NAME_LOWER}_${PROTO_SERVER_METHOD_NAME}(ngx_rpc_task_t* _this, void* p1)",
    "  {",
    "      ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)p1;",

    "      RpcChannel *cntl = new RpcChannel(rpc_ctx->r);",

    "      cntl->task = _this;",
    "      cntl->req  = ${PROTO_SERVER_METHOD_REQUEST_NAME}();",
    "      cntl->res  = ${PROTO_SERVER_METHOD_RESPONSE_NAME}();",

    "     cntl->done =  std::bind(&RpcChannel::finish_request,",
    "                               std::placeholders::_1,",
    "                               std::placeholders::_2,",
    "                             std::placeholders::_3,",
    "                              std::placeholders::_4);",


    "     NgxChainBufferReader reader(_this->req_bufs);",

    "      if(!cntl->req->ParseFromZeroCopyStream(&reader))",
    "      {",

    "        ngx_log_error(NGX_LOG_ERR, _this->log, 0, \"ParseFromZeroCopyStream req_bufs:%p %d\",",

    "                      _this->req_bufs.buf->pos, (_this->req_bufs.buf->last - _this->req_bufs.buf->pos));",

    "        cntl->done(cntl, cntl->req,cntl->res,NGX_HTTP_INTERNAL_SERVER_ERROR);",
    "        return;",
    "    }",

    "      ${PROTO_SERVER_NAME}Server * impl = (${PROTO_SERVER_NAME}Server *) rpc_ctx->method->_impl;",


    "      impl->${PROTO_SERVER_METHOD_NAME}(cntl,",
    "                      (${PROTO_SERVER_METHOD_REQUEST_NAME})cntl->req,",
    "                    (${PROTO_SERVER_METHOD_RESPONSE_NAME}*)cntl->res,",
    "                   cntl->done);"

};


