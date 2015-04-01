// C
extern "C" {
#include "ngx_http_rpc.h"
}

// for rpc server header
#include "inspect_impl.h"
#include "ngx_rpc_channel.h"


// associate the http request session
typedef struct {
   ngx_str_t name;
   void (*post_handler)(ngx_http_request_t *r);
} method_conf_t;

typedef struct {
    RpcChannel *cntl;
    ngxrpc::inspect::ApplicationServer* application_impl;
    ngx_log_t *log;
    method_conf_t* conf;
} ngx_http_inspect_ctx_t;


typedef struct
{
    ngx_log_t *log;
    ngx_hash_t * methods;
    // server
    ngxrpc::inspect::ApplicationServer* application_impl;
    // other servers go here
} ngx_http_inspect_conf_t;


static void ngxrpc_inspect_application_interface(ngx_http_request_t *r);
static void ngxrpc_inspect_application_requeststatus(ngx_http_request_t *r);




static  method_conf_t ngx_conf_set_inspect_application_methods[] = {
     {ngx_string("/ngxrpc/inspect/application/interface"), ngxrpc_inspect_application_interface},
     {ngx_string("/ngxrpc/inspect/application/requeststatus"), ngxrpc_inspect_application_interface}
};

static char *ngx_conf_set_inspect_application_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_conf_set_inspect_application_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

/* Commands */
static ngx_command_t ngx_http_inspect_commands[] = {
    { ngx_string("set_inspect_application_hanlder"),
      NGX_HTTP_LOC_CONF | NGX_CONF_ANY,
      ngx_conf_set_inspect_application_hanlder,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("set_inspect_application_log"),
      NGX_HTTP_LOC_CONF | NGX_CONF_ANY,
      ngx_conf_set_inspect_application_log,
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
    // 1 conf
    ngx_http_inspect_conf_t *conf = (ngx_http_inspect_conf_t *)
            ngx_pcalloc(cf->pool, sizeof(ngx_http_inspect_conf_t));

    if(conf == NULL)
    {
        ERROR("ngx_http_inspect_conf_t create failed");
        return NULL;
    }

    // 2 server objs
    conf->application_impl = (ngxrpc::inspect::ApplicationServer*)
            NGX_CONF_UNSET_PTR;

    // 3 log objs
    conf->log = (ngx_log_t*)ngx_pcalloc(cf->pool, sizeof(ngx_log_t));
    memset(conf->log, 0, sizeof(ngx_log_t));

    ngx_log_error(NGX_LOG_INFO, conf->log, 0, "ngx_http_inspect_create_loc_conf initi log");
    ngx_conf_log_error(NGX_LOG_INFO, cf, 0, "ngx_http_inspect_create_loc_conf done");


    // 4 methodes
    ngx_array_t* eles = ngx_array_create(cf->pool,
                            sizeof(ngx_conf_set_inspect_application_methods)/sizeof(ngx_conf_set_inspect_application_methods[0]),
                            sizeof(ngx_hash_key_t));

    ngx_hash_key_t *key = (ngx_hash_key_t *)ngx_array_push(eles);

    // 0
    key->key = ngx_conf_set_inspect_application_methods[0].name;
    key->key_hash = ngx_hash_key_lc(key->key.data, key->key.len);
    key->value = &(ngx_conf_set_inspect_application_methods[0]);

    // 1
    key->key = ngx_conf_set_inspect_application_methods[1].name;
    key->key_hash = ngx_hash_key_lc(key->key.data, key->key.len);
    key->value = &(ngx_conf_set_inspect_application_methods[1]);


    ngx_hash_init_t hash_init = {
        NULL,
        &ngx_hash_key_lc,
        4,
        64,
        "application_methods",
        cf->pool,
        NULL
    };

    ngx_int_t ret = ngx_hash_init(&hash_init, (ngx_hash_key_t*)eles->elts, elements->nelts);

    if(ret != NGX_OK){

        return NULL;
    }

    conf->methods = hash_init.hash;

    ngx_array_destroy(eles);
    return conf;
}

static ngx_int_t ngx_http_inspect_http_handler(ngx_http_request_t *r);

static char *ngx_conf_set_inspect_application_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_inspect_conf_t *c = (ngx_http_inspect_conf_t*) conf;

    c->application_impl = new ngxrpc::inspect::ApplicationServer();
    // Add some init

    ngx_http_core_loc_conf_t *clcf = (ngx_http_core_loc_conf_t *)
            ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    clcf->handler = ngx_http_inspect_http_handler;

    ngx_conf_log_error(NGX_LOG_INFO, cf, 0, "ngx_conf_set_inspect_application_hanlder done");

    return NGX_OK;
}


static char *ngx_conf_set_inspect_application_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_inspect_conf_t *inspect_conf = (ngx_http_inspect_conf_t*) conf;

    ngx_str_t *value = (ngx_str_t *)cf->args->elts;

    inspect_conf->log->file = ngx_conf_open_file(cf->cycle, &value[1]);
    inspect_conf->log->log_level = NGX_DEBUG;

    ngx_log_error(NGX_LOG_INFO, inspect_conf->log, 0, "ngx_conf_set_inspect_application_log init");

    ngx_conf_log_error(NGX_LOG_INFO, cf, 0, "ngx_conf_set_inspect_application_log done");

    return NGX_OK;
}

static void ngx_inspect_process_exit(ngx_cycle_t* cycle)
{
    ngx_http_conf_ctx_t * ctx = (ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index];
    ngx_http_inspect_conf_t *c = (ngx_http_inspect_conf_t *) ctx ->loc_conf[ngx_http_module.ctx_index];

    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "ngx_inspect_process_exit done");
    delete c->application_impl;
}




void ngx_http_inspect_application_interface_done(ngx_rpc_task_t *task,RpcChannel *channel,
                                                 const ::google::protobuf::Message* req,
                                                 ::google::protobuf::Message* res,
                                                 int result)
{

    ngx_http_request_t *r =(ngx_http_request_t *) channel->r;

    task->response_states = result;

    if(res && task->type != PROCESS_IN_PROC)
    {
        NgxChainBufferWriter writer(task->res_bufs, r->pool);
        res->SerializeToZeroCopyStream(&writer);

        ngx_http_rpc_ctx_finish_by_task(task);
        return;
    }

    if(res && task->type == PROCESS_IN_PROC)
    {
        ngx_http_rpc_ctx_t *ctx_conf = (ngx_http_rpc_ctx_t *)task->ctx;
        NgxShmChainBufferWriter writer(task->res_bufs, task->pool);
        res->SerializeToZeroCopyStream(&writer);

        ngx_rpc_notify_task(ctx_conf->notify, ngx_http_rpc_ctx_finish_by_task, task);
        return;
    }

    ngx_http_rpc_ctx_finish_by_task(task);
}



static void ngx_http_inspect_application_interface_handler(ngx_rpc_task_t *task, void* ctx )
{
    ngx_http_request_t *r =(ngx_http_request_t *)ctx;
    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_inspect_module);

    RpcChannel* cntl = inspect_ctx->cntl;
    cntl->task = task;
    cntl->req        = new ngxrpc::inspect::Request();


    NgxChainBufferReader reader(task->req_bufs);

    if(!cntl->req->ParseFromZeroCopyStream(&reader))
    {
        ngx_http_inspect_application_interface_done(NULL,
                                                    cntl,
                                                    NULL,
                                                    NULL,
                                                    NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    cntl->res        = new ngxrpc::inspect::Response();

    RpcCallHandler done = std::bind(ngx_http_inspect_application_interface_done, task,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4);

    inspect_ctx->application_impl->interface(inspect_ctx->cntl, (ngxrpc::inspect::Request*)cntl->req, (ngxrpc::inspect::Response*)cntl->res, done);

}

// this function call by nginx is one by one so when this call,the pre task is done
static void ngx_http_inspect_post_async_handler(ngx_http_request_t *r)
{
    if(r->request_body == NULL
            || r->request_body->bufs == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Request body is NULL");
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }

    ngx_http_rpc_ctx_t *ctx_conf = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    ngx_rpc_task_t *task = ngx_http_rpc_post_request_task_init(r, ctx_conf);

    // router use hash
    if(0 == strncasecmp("/ngxrpc.inspect.application.interface",
                        (const char*)r->uri.data, r->uri.len))
    {
        task->filter = ngx_http_inspect_application_interface_handler;
        task->type   = PROCESS_IN_PROC;
    }

    if(0 == strncasecmp("/ngxrpc/inspect/application.interface",
                        (const char*)r->uri.data, r->uri.len))
    {
        task->filter = ngx_http_inspect_application_interface_handler;
        task->type   = PROCESS_IN_PROC;
    }

    if(0 == strncasecmp("/ngxrpc.inspect.application.requeststatus",
                        (const char*)r->uri.data, r->uri.len))
    {
        task->filter = ngx_http_inspect_application_interface_handler;
        task->type  = PROCESS_IN_SUBREQUEST;
    }

    if(0 == strncasecmp("/ngxrpc/inspect/application/requeststatus",
                        (const char*)r->uri.data, r->uri.len))
    {
        task->filter = ngx_http_inspect_application_interface_handler;
        task->type  = PROCESS_IN_SUBREQUEST;
    }

    if(task->filter == NULL )
    {
        ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
        return;
    }

    // 2 dispatch task
    ngx_http_rpc_dispatcher_task(task);
}

static ngx_int_t ngx_http_inspect_http_handler(ngx_http_request_t *r)
{
    // 0 find the hanlder

    // 1 init the ctx
    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_inspect_module);
    ngx_http_inspect_conf_t *inspect_conf = (ngx_http_inspect_conf_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_inspect_module);
    ngx_http_rpc_conf_t *rpc_conf = (ngx_http_rpc_conf_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_rpc_module);

    //2
    if(rpc_conf == NULL || rpc_conf->shpool == NULL)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "Method:%s,url:%V rpc_conf:%p!",
                      r->request_start, &(r->uri), rpc_conf);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if(inspect_ctx == NULL)
    {
        //do something
        inspect_ctx = (ngx_http_inspect_ctx_t *)
                ngx_slab_alloc(rpc_conf->shpool, sizeof(ngx_http_inspect_ctx_t));

        if(inspect_ctx == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "ngx_palloc error size:%d",
                          sizeof(ngx_http_inspect_ctx_t));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        RpcChannel *cntl =  new RpcChannel(r);
        ngx_pool_cleanup_t *p2 = ngx_pool_cleanup_add(r->connection->pool, 0);
        p2->data = cntl;
        p2->handler = RpcChannel::destructor;

        inspect_ctx->cntl = cntl;
        inspect_ctx->application_impl = inspect_conf->application_impl;
        inspect_ctx->log = inspect_conf->log;
        ngx_http_set_ctx(r, inspect_ctx, ngx_http_inspect_module);

        ngx_log_error(NGX_LOG_INFO,inspect_ctx->log, 0, "ngx_http_inspect_http_handler init the inspect_ctx");
    }

    if (ngx_http_get_module_ctx(r, ngx_http_rpc_module) == NULL)
    {
        ngx_http_rpc_ctx_t *rpc_ctx = ngx_http_rpc_ctx_init(r, inspect_ctx);
        rpc_ctx->timeout_ms = ngx_current_msec;
        rpc_ctx->log  = inspect_ctx->log;

        ngx_log_error(NGX_LOG_INFO,inspect_ctx->log, 0, "ngx_http_inspect_http_handler init the rpc_ctx");
    }

    ngx_log_t *log_ptr = r->connection->log;
    for(;log_ptr->next != NULL; log_ptr=log_ptr->next );
    log_ptr->next = inspect_ctx->log;

    // 2 forward to the post handler
    ngx_int_t rc = ngx_http_read_client_request_body(r, ngx_http_inspect_post_async_handler);

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "Method:%s,url:%V rc:%d!",
                      r->request_start, &(r->uri), rc);
    }

    return rc;
}











