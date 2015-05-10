// C
extern "C" {
    #include "ngx_http_rpc.h"
}

// for rpc server header
#include "inspect_impl.h"
#include "ngx_rpc_channel.h"


using namespace ngxrpc::inspect;

typedef struct
{
    // application server
    ngx_str_t  application_conf_file;
    ngx_str_t  application_log_file;
    ApplicationServer * application_impl;
    ngx_log_t*          application_log;

} ngx_http_inspect_conf_t;



static char *ngx_conf_set_inspect_application_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

/* Commands */
static ngx_command_t ngx_http_inspect_commands[] = {
    { ngx_string("set_inspect_application_hanlder"),
      NGX_HTTP_LOC_CONF | NGX_CONF_ANY,
      ngx_conf_set_inspect_application_hanlder,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};

static void* ngx_http_inspect_create_loc_conf(ngx_conf_t *cf);

/* Modules */
static ngx_http_module_t ngx_http_inspect_module_ctx = {
    NULL,                /* preconfiguration */
    NULL,                /* postconfiguration */

    NULL,                /* create main configuration */
    NULL,                /* init main configuration */

    NULL,                /* create server configuration */
    NULL,                /* merge server configuration */

    ngx_http_inspect_create_loc_conf,
    /* create location configuration */
    NULL                 /* merge location configuration */
};


static void ngx_inspect_process_exit(ngx_cycle_t* cycle);


extern "C" {

ngx_module_t ngx_http_inspect_module = {
    NGX_MODULE_V1,
    &ngx_http_inspect_module_ctx,              /* module context */
    ngx_http_inspect_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but some where called init module instead
    NULL,             /* init module */
    NULL,             /* init process */
    NULL,             /* init thread */
    NULL,             /* exit thread */
    ngx_inspect_process_exit, /* exit process */
    NULL,                     /* exit master */
    NGX_MODULE_V1_PADDING
};

}


////
static void* ngx_http_inspect_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_inspect_conf_t *conf = (ngx_http_inspect_conf_t *)
            ngx_pcalloc(cf->pool, sizeof(ngx_http_inspect_conf_t));

    conf->application_conf_file = ngx_string("");
    conf->application_log_file = ngx_string("");

    return conf;
}


static void ngxrpc_inspect_application_interface(ngx_rpc_task_t* _this, void* p1)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)p1;

    RpcChannel *cntl = new RpcChannel(rpc_ctx->r);

    cntl->task = _this;
    cntl->req  = new ngxrpc::inspect::Request();
    cntl->res  = new ngxrpc::inspect::Response();

    cntl->done =  std::bind(&RpcChannel::finish_request,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            std::placeholders::_4);


    NgxChainBufferReader reader(_this->req_bufs);

    if(!cntl->req->ParseFromZeroCopyStream(&reader))
    {

        ngx_log_error(NGX_LOG_ERR, _this->log, 0, "ParseFromZeroCopyStream req_bufs:%p %d",

                      _this->req_bufs.buf->pos, (_this->req_bufs.buf->last - _this->req_bufs.buf->pos));

        cntl->done(cntl, cntl->req,cntl->res,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ApplicationServer * impl = (ApplicationServer *) rpc_ctx->method->_impl;


    impl->interface(cntl,
                    (ngxrpc::inspect::Request*)cntl->req,
                    (ngxrpc::inspect::Response*)cntl->res,
                    cntl->done);

    ngx_log_error(NGX_LOG_INFO, _this->log, 0, " ngxrpc_inspect_application_interface process : impl:%p cntl:%p req:%p res:%p task:%p", impl, cntl, cntl->req, cntl->res, _this);
}





static void ngxrpc_inspect_application_requeststatus(ngx_rpc_task_t* _this, void* p1)
{

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)p1;

    RpcChannel *cntl = new RpcChannel(rpc_ctx->r);

    cntl->task = _this;
    cntl->req  = new ngxrpc::inspect::Request();
    cntl->res  = new ngxrpc::inspect::Response();

    cntl->done =  std::bind(&RpcChannel::finish_request,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            std::placeholders::_4);


    NgxChainBufferReader reader(_this->req_bufs);

    if(!cntl->req->ParseFromZeroCopyStream(&reader))
    {

        ngx_log_error(NGX_LOG_ERR, _this->log, 0, "ParseFromZeroCopyStream req_bufs:%p %d",

                      _this->req_bufs.buf->pos, (_this->req_bufs.buf->last - _this->req_bufs.buf->pos));

        cntl->done(cntl, cntl->req,cntl->res,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ApplicationServer * impl = (ApplicationServer *) rpc_ctx->method->_impl;


    impl->requeststatus(cntl,
                    (ngxrpc::inspect::Request*)cntl->req,
                    (ngxrpc::inspect::Response*)cntl->res,
                    cntl->done);

   ngx_log_error(NGX_LOG_INFO, _this->log, 0, " ngxrpc_inspect_application_interface process : impl:%p cntl:%p req:%p res:%p task:%p", impl, cntl, cntl->req, cntl->res, _this);
}



static char* ngx_conf_set_inspect_application_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{

    ngx_http_rpc_conf_t *rpc_conf = (ngx_http_rpc_conf_t *)
            ngx_http_conf_get_module_main_conf(cf, ngx_http_rpc_module);

    if(rpc_conf == NULL)
    {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "ngx_http_rpc_module not init");
        return (char*)"ngx_http_rpc_module not init";
    }

    ngx_http_inspect_conf_t *inspect_conf = (ngx_http_inspect_conf_t*) conf;
    ngx_str_t* value = (ngx_str_t*)cf->args->elts;

    if(cf->args->nelts > 1)
    {
         inspect_conf->application_conf_file = (value[1]);
    }

    const std::string app_conf((const char *)inspect_conf->application_conf_file.data);
    inspect_conf->application_impl = new ApplicationServer(app_conf);


    inspect_conf->application_log = ngx_cycle->log;

    if( cf->args->nelts > 2)
    {
       inspect_conf->application_log_file = (value[2]);
       inspect_conf->application_log  = (ngx_log_t*)ngx_palloc(cf->pool, sizeof(ngx_log_t));
       inspect_conf->application_log->file = ngx_conf_open_file(cf->cycle, &value[1]);
       inspect_conf->application_log->log_level = NGX_DEBUG;
    }


    {
        method_conf_t *method = (method_conf_t *)ngx_array_push(rpc_conf->method_array);
        method->name    = ngx_string("/ngxrpc/inspect/Application/interface");
        method->_impl   = inspect_conf->application_impl;
        method->handler = ngxrpc_inspect_application_interface;
        method->log     = inspect_conf->application_log;
    }

    {
        method_conf_t *method = (method_conf_t *)ngx_array_push(rpc_conf->method_array);
        method->name    = ngx_string("/ngxrpc/inspect/Application/requeststatus");
        method->_impl   = inspect_conf->application_impl;
        method->handler = ngxrpc_inspect_application_requeststatus;
        method->log     = inspect_conf->application_log;
    }


    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                  "ngx_conf_set_inspect_application_hanlder with application_impl:%p, conf file:%V log_file:%V",
                  inspect_conf->application_impl,
                  &inspect_conf->application_conf_file,
                  &inspect_conf->application_log_file);


    return NGX_OK;
}


static void ngx_inspect_process_exit(ngx_cycle_t* cycle)
{
    ngx_http_conf_ctx_t * ctx = (ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index];
    ngx_http_inspect_conf_t *c = (ngx_http_inspect_conf_t *) ctx ->loc_conf[ngx_http_module.ctx_index];

    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "ngx_inspect_process_exit done");

    delete c->application_log;
}

