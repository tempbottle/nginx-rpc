// C
extern "C" {
#include "ngx_http_rpc.h"
}

// for rpc server header
#include "inspect_impl.h"
#include "ngx_rpc_channel.h"


// associate the http request session


typedef struct {
    ngx_http_rpc_conf_t*  rpc_conf;
    ngx_log_t *log;

    ngx_http_request_t *r;

    ngxrpc::inspect::ApplicationServer* application_impl;
    RpcChannel *cntl;
    method_conf_t *mth;

} ngx_http_inspect_ctx_t;


static void ngx_http_inspect_ctx_destroy(void* ctx){

    ngx_http_inspect_ctx_t *c = (ngx_http_inspect_ctx_t *)ctx;
    ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "free the ngx_http_inspect_ctx_t:%p", ctx);
    ngx_slab_free(c->rpc_conf->proc_queue->pool ,c);
}

typedef struct
{
    ngx_log_t *log;
    ngx_hash_t * methods;
    // server
    ngxrpc::inspect::ApplicationServer* application_impl;
    // other servers go here
} ngx_http_inspect_conf_t;


// method closure
static void ngxrpc_inspect_application_interface(ngx_rpc_task_t *_this, void *p1);



static void ngxrpc_inspect_application_requeststatus(ngx_rpc_task_t *_this, void *p1);

#define inspect_application_method_num  (sizeof(ngx_conf_set_inspect_application_methods)\
    /sizeof(ngx_conf_set_inspect_application_methods[0]))

static  method_conf_t ngx_conf_set_inspect_application_methods[] = {
    {ngx_string("/ngxrpc/inspect/application/interface"), ngxrpc_inspect_application_interface},
    {ngx_string("/ngxrpc/inspect/application/requeststatus"), ngxrpc_inspect_application_requeststatus}
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



    // 4 methodes
    ngx_array_t* eles = ngx_array_create(cf->pool,inspect_application_method_num,
                                         sizeof(ngx_hash_key_t));


    for(unsigned int i = 0; i< inspect_application_method_num; ++i)
    {
        ngx_hash_key_t *key = (ngx_hash_key_t *)ngx_array_push(eles);
        key->key = ngx_conf_set_inspect_application_methods[i].name;
        key->key_hash = ngx_hash_key_lc(key->key.data, key->key.len);
        key->value = &(ngx_conf_set_inspect_application_methods[i]);
    }


    ngx_hash_init_t hash_init = {
        NULL,
        &ngx_hash_key_lc,
        4,
        64,
        (char*)"application_methods",
        cf->pool,
        NULL
    };

    ngx_int_t ret = ngx_hash_init(&hash_init, (ngx_hash_key_t*)eles->elts, eles->nelts);

    if(ret != NGX_OK){
        ngx_array_destroy(eles);
        return NULL;
    }

    conf->methods = hash_init.hash;
    ngx_array_destroy(eles);
    for(unsigned int i = 0; i< inspect_application_method_num; ++i)
    {
        ngx_log_error(NGX_LOG_INFO, conf->log, 0, "ngx_http_inspect_create_loc_conf add:%V",
                      ngx_conf_set_inspect_application_methods[i].name);
    }
    return conf;
}

static void LogHandler(google::protobuf::LogLevel level, const char* filename, int line,
                        const std::string& message)
{
     ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "file:%s line:%d message:%s",
                   filename, line, message.c_str());
}

/// 1 init some configure
static ngx_int_t ngx_http_inspect_http_handler(ngx_http_request_t *r);

static char *ngx_conf_set_inspect_application_hanlder(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_inspect_conf_t *c = (ngx_http_inspect_conf_t*) conf;

    // add some conf
    c->application_impl = new ngxrpc::inspect::ApplicationServer();

    // Add some init

    ngx_http_core_loc_conf_t *clcf = (ngx_http_core_loc_conf_t *)
            ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    clcf->handler = ngx_http_inspect_http_handler;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0,
                  "ngx_conf_set_inspect_application_hanlder for ngxrpc::inspect::ApplicationServer");

    ::google::protobuf::SetLogHandler(LogHandler);

    return NGX_OK;
}


static char *ngx_conf_set_inspect_application_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_inspect_conf_t *inspect_conf = (ngx_http_inspect_conf_t*) conf;

    ngx_str_t *value = (ngx_str_t *)cf->args->elts;

    inspect_conf->log->file = ngx_conf_open_file(cf->cycle, &value[1]);
    inspect_conf->log->log_level = NGX_DEBUG;

    ngx_log_error(NGX_LOG_INFO, inspect_conf->log, 0, "ngx_conf_set_inspect_application_log init");
    return NGX_OK;
}


static void ngx_inspect_process_exit(ngx_cycle_t* cycle)
{
    ngx_http_conf_ctx_t * ctx = (ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index];
    ngx_http_inspect_conf_t *c = (ngx_http_inspect_conf_t *) ctx ->loc_conf[ngx_http_module.ctx_index];

    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "ngx_inspect_process_exit done");

    delete c->application_impl;
}





static void ngx_http_inspect_post_async_handler(ngx_http_request_t *r)
{
    if(r->request_body == NULL
            || r->request_body->bufs == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Request body is NULL");
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }


    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_inspect_module);

    ngx_slab_pool_t*pool = inspect_ctx->rpc_conf->proc_queue->pool;
    ngx_rpc_task_t* task = ngx_http_rpc_task_create(pool, inspect_ctx->log);

    task->done_notify = inspect_ctx->rpc_conf->notify;

    // 2 copy the request bufs
    ngx_http_rpc_task_set_bufs(task->pool, &task->req_bufs, r->request_body->bufs);
    task->res_length = r->headers_in.content_length_n;

    // processor
    task->closure.handler = inspect_ctx->mth->handler;
    task->closure.p1  =  inspect_ctx;

    // push queue
    int ret = ngx_rpc_queue_push_and_notify(inspect_ctx->rpc_conf->proc_queue, task);

    if(ret != NGX_OK)
    {
        task->response_states = NGX_HTTP_INTERNAL_SERVER_ERROR;
        task->res_length = 0;

        ngx_http_rpc_request_finish(task, r);
    }
}


/// the main hanlder when the request come in.
/// nginx called this method with:
///     if (r->content_handler) {
///           r->write_event_handler = ngx_http_request_empty_handler;
///           ngx_http_finalize_request(r, r->content_handler(r));
///           return NGX_OK;
///      }
/// so this method just return the status
static ngx_int_t ngx_http_inspect_http_handler(ngx_http_request_t *r)
{
    // 1 init the ctx

    ngx_http_inspect_conf_t *inspect_conf = (ngx_http_inspect_conf_t *)
            ngx_http_get_module_loc_conf(r, ngx_http_inspect_module);

    ngx_http_rpc_conf_t *rpc_conf = (ngx_http_rpc_conf_t *)
            ngx_http_get_module_main_conf(r, ngx_http_rpc_module);

    //2
    if(rpc_conf == NULL || rpc_conf->proc_queue == NULL)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "Method:%s,url:%V rpc_conf:%p !",
                      r->request_start, &(r->uri), rpc_conf);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if(inspect_conf->log->file)
    {
        ngx_log_t *log_ptr = r->connection->log;
        for(;log_ptr->next != NULL; log_ptr=log_ptr->next );
        log_ptr->next = inspect_conf->log;
    }


    // 0 find the hanlder if not found termial the request
    method_conf_t *mth = (method_conf_t*) ngx_hash_find(
                inspect_conf->methods,
                ngx_hash_key_lc(r->uri.data, r->uri.len),
                r->uri.data, r->uri.len);

    if(mth == NULL)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "Method:%s,url:%V Not Found",
                      r->request_start, &(r->uri));
        return NGX_HTTP_NOT_FOUND;
    }

    // 1 when a new request call in ,init the ctx
    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_inspect_module);

    if(inspect_ctx == NULL)
    {
        //do something
        inspect_ctx = (ngx_http_inspect_ctx_t *) ngx_slab_alloc(
                    rpc_conf->proc_queue->pool, sizeof(ngx_http_inspect_ctx_t));

        if(inspect_ctx == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "ngx_palloc error size:%d",
                          sizeof(ngx_http_inspect_ctx_t));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        inspect_ctx->rpc_conf = rpc_conf;
        inspect_ctx->r = r;


        // the destructor of ngx_http_inspect_ctx_t
        ngx_pool_cleanup_t *p1 = ngx_pool_cleanup_add(r->connection->pool, 0);
        p1->data = inspect_ctx;
        p1->handler = ngx_http_inspect_ctx_destroy;

        inspect_ctx->cntl = NULL;
        inspect_ctx->application_impl = inspect_conf->application_impl;

        inspect_ctx->mth = mth;

        inspect_ctx->log = inspect_conf->log->file== NULL ?
                    r->connection->log : ngx_cycle->log;

        ngx_http_set_ctx(r, inspect_ctx, ngx_http_inspect_module);
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "ngx_http_inspect_http_handler init the inspect_ctx");
    }

    // 2 forward to the post handler
    ngx_int_t rc = ngx_http_read_client_request_body(r,
                                               ngx_http_inspect_post_async_handler);

    ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "Method:%V, url:%V rc:%d!",
                  &r->method_name, &(r->uri), rc);

    return NGX_OK;
}


// 3
static void ngxrpc_inspect_application_interface(ngx_rpc_task_t* _this, void* p1)
{
    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)p1;

    inspect_ctx->cntl = new RpcChannel(inspect_ctx->r);

    inspect_ctx->cntl->task = _this;
    inspect_ctx->cntl->req  = new ngxrpc::inspect::Request();


    NgxChainBufferReader reader(_this->req_bufs);

    if(!inspect_ctx->cntl->req->ParseFromZeroCopyStream(&reader))
    {

        ngx_log_error(NGX_LOG_ERR, inspect_ctx->log, 0, "ParseFromZeroCopyStream req_bufs:%p %d",

                      _this->req_bufs.buf->pos, (_this->req_bufs.buf->last - _this->req_bufs.buf->pos));

        inspect_ctx->cntl->done(inspect_ctx->cntl, inspect_ctx->cntl->req,
                                inspect_ctx->cntl->res,
                                NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    inspect_ctx->cntl->res = new ngxrpc::inspect::Response();

    inspect_ctx->cntl->done =  std::bind(&RpcChannel::finish_request,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            std::placeholders::_4);

    inspect_ctx->application_impl->interface(inspect_ctx->cntl,
                                             (ngxrpc::inspect::Request*)inspect_ctx->cntl->req,
                                             (ngxrpc::inspect::Response*)inspect_ctx->cntl->res,
                                             inspect_ctx->cntl->done);
}





static void ngxrpc_inspect_application_requeststatus(ngx_rpc_task_t* _this, void* p1)
{

    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)p1;

    inspect_ctx->cntl = new RpcChannel(inspect_ctx->r);

    inspect_ctx->cntl->task = _this;
    inspect_ctx->cntl->req  = new ngxrpc::inspect::Request();
    inspect_ctx->cntl->res = new ngxrpc::inspect::Response();

    inspect_ctx->cntl->done =  std::bind(&RpcChannel::finish_request,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            std::placeholders::_4);


    NgxChainBufferReader reader(_this->req_bufs);

    if(!inspect_ctx->cntl->req->ParseFromZeroCopyStream(&reader))
    {

        ngx_log_error(NGX_LOG_ERR, inspect_ctx->log, 0, "ParseFromZeroCopyStream req_bufs:%p %d",

                      _this->req_bufs.buf->pos, (_this->req_bufs.buf->last - _this->req_bufs.buf->pos));

        inspect_ctx->cntl->done(inspect_ctx->cntl, inspect_ctx->cntl->req,
                                inspect_ctx->cntl->res,
                                NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }


    inspect_ctx->application_impl->requeststatus(inspect_ctx->cntl,
                                             (ngxrpc::inspect::Request*)inspect_ctx->cntl->req,
                                             (ngxrpc::inspect::Response*)inspect_ctx->cntl->res,
                                             inspect_ctx->cntl->done);
}



