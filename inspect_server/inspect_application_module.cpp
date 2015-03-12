// common header
#include "ngx_log_cpp.h"

#include "ngx_rpc_module/ngx_rpc_buffer.h"

// for rpc server header
#include "inspect_application_impl.h"
#include "ngx_rpc_server.h"
// associate the http request

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    void *cntl;

} ngx_http_inspect_application_ctx_t;


typedef struct
{
    bool async;

} ngx_http_inspect_application_conf_t;



static void*     ngx_http_inspect_application_create_loc_conf(ngx_conf_t *cf);


// this function link to nginx

static ngx_int_t ngx_http_inspect_application_master_init(ngx_cycle_t *cycle);
static void      ngx_http_inspect_application_master_exit(ngx_cycle_t *cycle);

static ngx_int_t ngx_http_inspect_application_process_init(ngx_cycle_t *cycle);
static void      ngx_http_inspect_application_process_exit(ngx_cycle_t *cycle);



/* Commands */
static ngx_command_t ngx_http_inspect_application_commands[] = {
    { ngx_string("inspect_application_handler"),
      NGX_HTTP_SRV_CONF | NGX_CONF_NOARGS,
      ngx_conf_set_flag_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_inspect_application_conf_t, async),
      NULL },

    ngx_null_command
};


/* Modules */
static ngx_http_module_t ngx_http_inspect_application_module_ctx = {
    NULL,                /* preconfiguration */
    NULL,                /* postconfiguration */

    NULL,                /* create main configuration */
    NULL,                /* init main configuration */

    NULL,                /* create server configuration */
    NULL,                /* merge server configuration */

    ngx_http_inspect_application_create_loc_conf,
    /* create location configuration */
    NULL                 /* merge location configuration */
};

ngx_module_t ngx_http_inspect_application_module = {
    NGX_MODULE_V1,
    &ngx_http_inspect_application_module_ctx,              /* module context */
    ngx_http_rpc_module_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but some where called init module instead
    ngx_http_inspect_application_master_init,              /* init module */
    ngx_http_inspect_application_process_init,             /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_http_inspect_application_process_exit,             /* exit process */
    ngx_http_inspect_application_master_exit,             /* exit master */
    NGX_MODULE_V1_PADDING
};


#ifdef __cplusplus
}
#endif

///impl

static ApplicationServiceImpl* impl = NULL;


// TODO support with reopen
static ngx_int_t ngx_http_inspect_application_master_init(ngx_cycle_t *cycle)
{
    impl = new ApplicationServiceImpl();
    set_cache_log(cycle);
    INFO("ngx_http_inspect_application_master_init");
    return 0;
}

static void     ngx_http_inspect_application_master_exit(ngx_cycle_t *cycle)
{
    delete impl;
    INFO("ngx_http_inspect_application_master_exit");
}

static ngx_int_t ngx_http_inspect_application_process_init(ngx_cycle_t *cycle)
{
    INFO("ngx_http_inspect_application_master_exit");
    return 0;
}

static void  ngx_http_inspect_application_process_exit(ngx_cycle_t *cycle)
{
    INFO("ngx_http_inspect_application_master_exit");
}


static void  ngx_http_inspect_application_finish_request(void* cn)
{
    // 1 save res to chain buffer

    NgxRpcController *cntl = (NgxRpcController *)(((ngx_http_inspect_application_ctx_t* )cn)->cntl);

    ngx_chain_t chain;
    ngx_http_request_t * r = cntl->r;

    NgxChainBufferWriter writer(chain, r->pool);

    if(!cntl->res->SerializeToZeroCopyStream(&writer))
    {
        ERROR("SerializeToZeroCopyStream failed:"<<cntl->res->GetTypeName());
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR );
        return;
    }

    // 2 send header & body
    static ngx_str_t type = ngx_string(" application/x-protobuf");

    r->headers_out.content_type = type;
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = cntl->res->ByteSize();
    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;


    ngx_int_t rc = ngx_http_send_header(r);
    rc = ngx_http_output_filter(r, &chain);
    ngx_http_finalize_request(r, rc);
}

static void ngx_http_inspect_application_async_request(void* cn)
{

    NgxRpcController *cntl = (NgxRpcController *)(((ngx_http_inspect_application_ctx_t* )cn)->cntl);


    ngx_http_request_t* r =  cntl->r;

    ::google::protobuf::Message* req = impl->GetRequestPrototype(mdp).New();

    ::google::protobuf::Message* res = impl->GetResponsePrototype(mdp).New();

    cntl->reset( req, res);

    NgxChainBufferReader reader(*r->request_body->bufs);

    if(!req->ParseFromZeroCopyStream(&reader))
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log,
                      0,
                      "parser request fialed:%V, msg name:%s",
                      &r->uri, req->GetTypeName().c_str());

        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }

    ::google::protobuf::Closure *done =
            ::google::protobuf::NewCallback(
                ngx_http_inspect_application_finish_request, cntl);


    const_cast< ::google::protobuf::Service*>(impl)->CallMethod(mdp, cntl, req, res, done);


}

static void ngx_http_inspect_application_post_handler(ngx_http_request_t *r)
{
    if(r->request_body == NULL
            || r->request_body->bufs == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Request body is NULL");
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }


    ngx_http_inspect_application_ctx_t *ctx =
            ( ngx_http_inspect_application_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_inspect_application_module);

    if(0 == strncasecmp("/ngxrpc.inspect.Application",r->uri.data,r->uri.len)
            || 0 == strncasecmp("/ngxrpc/inspect/Application",r->uri.data,r->uri.len) )
    {


    }


    const char* method_name = basename(r->uri.data);
    const ::google::protobuf::MethodDescriptor* mdp = impl->GetDescriptor()->FindMethodByName(method_name);

    if(mdp == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log,
                      0,
                      "parser request fialed:%V, method_name:%s",
                      &r->uri, method_name);

        ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
        return;
    }

    INFO("method_name:"<<method_name<<", url:"<<r->uri.data);
    NgxRpcController * cntl = ctx->cntl;
    cntl->reset(req ,res);
    cntl->mdp = mdp;
    cntl->srv = impl;

    ngx_http_inspect_application_conf_t *conf = ngx_http_get_module_loc_conf(r, ngx_http_inspect_application_conf_t);

    if(conf->async != true)
    {
        ngx_http_inspect_application_async_request(ctx);
        return ;
    }

    // start a new async call






}

static void ngx_http_inspect_application_http_handler(ngx_http_request_t *r)
{
    // 1 only process the POST
    if (r->method != NGX_HTTP_POST)
    {
        return NGX_DECLINED;
    }

    // 2 init the ctx
    ngx_http_inspect_application_ctx_t *ctx =
            ( ngx_http_inspect_application_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_inspect_application_module);

    if (ctx == NULL)
    {
        ctx = (ngx_http_inspect_application_ctx_t *)
                ngx_palloc(r->pool, sizeof(ngx_http_inspect_application_ctx_t));

        if(ctx == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "ngx_palloc error size:%d",
                          sizeof(ngx_http_inspect_application_ctx_t));

            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        NgxRpcController * cntl =  new NgxRpcController(r);
        ctx->cntl =cntl;

        ngx_pool_cleanup_t * cln = ngx_pool_cleanup_add(r->pool, 0);
        cln->handler = NgxRpcController::clean_up_crontller;
        cln->data = ctx->cntl;

        //TODO init the ctx here
        ngx_http_set_ctx(r, ctx, ngx_http_inspect_application_module);
    }


    // 3 forward the post handler
    ngx_int_t rc = ngx_http_read_client_request_body(r,
                                                     ngx_http_inspect_application_post_handler);

    if (rc >=  NGX_HTTP_SPECIAL_RESPONSE)
    {
        ngx_log_error(NGX_LOG_WARN,
                      r->connection->log,
                      0,
                      "Method:%s,url:%V rc:%d!",
                      r->request_start,
                      &(r->uri), rc);
        return rc;
    }

    return NGX_OK;
}


static void* ngx_http_inspect_application_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_inspect_application_conf_t *conf =
            (ngx_http_inspect_application_conf_t *)
            ngx_pcalloc(cf->pool,                                                                                                sizeof(ngx_http_inspect_application_conf_t));

    if(conf == NULL)
    {
        ERROR("ngx_http_inspect_application_conf_t create failed");
        return NULL;
    }

    conf->enable = NGX_CONF_UNSET;

    //
    ngx_http_core_loc_conf_t *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler =  ngx_http_inspect_application_http_handler;

    return conf;

}
