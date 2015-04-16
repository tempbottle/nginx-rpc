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
    ngx_slab_free(c->queue->pool ,c);
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

static void ngx_http_inspect_application_interface_done(ngx_rpc_task_t *task,RpcChannel *channel,
                                                 const ::google::protobuf::Message* req,
                                                 ::google::protobuf::Message* res,
                                                 int result);

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

    ngx_rpc_task_t* task = ngx_http_rpc_task_create(inspect_ctx->queue->pool, inspect_ctx->log);

    task->log = inspect_ctx->log;
    task->notify = inspect_ctx->queue->notify_slot[inspect_ctx->proc_id];

    // 2 copy the request bufs

    ngx_chain_t* req_chain = &task->req_bufs;
    ngx_chain_t* c = NULL;

    for( c= r->request_body->bufs; c; c=c->next )
    {
        int buf_size = c->buf->last - c->buf->pos;
        req_chain->buf = (ngx_buf_t*)ngx_slab_alloc(inspect_ctx->queue->pool,
                                                           sizeof(ngx_buf_t));

        memcpy(req_chain->buf, c->buf,sizeof(ngx_buf_t));

        req_chain->buf->pos = req_chain->buf->start =
                (u_char*)ngx_slab_alloc(inspect_ctx->queue->pool, buf_size);

        memcpy(req_chain->buf->pos,c->buf->pos, buf_size);

        req_chain->next = (ngx_chain_t*)ngx_slab_alloc(inspect_ctx->queue->pool,
                                                              sizeof(ngx_chain_t));
        req_chain = req_chain->next;
        req_chain->next = NULL;
    }

    task->closure.handler = inspect_ctx->mth->handler;

    // 3 dispacher
    ngx_rpc_process_push_task(task);
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


        // the destructor of ngx_http_inspect_ctx_t
        ngx_pool_cleanup_t *p1 = ngx_pool_cleanup_add(r->connection->pool, 0);
        p1->data = inspect_ctx;
        p1->handler = ngx_http_inspect_ctx_destroy;

        inspect_ctx->cntl = NULL;
        inspect_ctx->application_impl = inspect_conf->application_impl;

        inspect_ctx->mth = mth;

        inspect_ctx->log = inspect_conf->log->file== NULL ?
                                 r->connection->log : inspect_conf->log;

        ngx_http_set_ctx(r, inspect_ctx, ngx_http_inspect_module);
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "ngx_http_inspect_http_handler init the inspect_ctx");
    }

    // 2 forward to the post handler
    ngx_int_t rc = ngx_http_read_client_request_body(r, ngx_http_inspect_post_async_handler);

    ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "Method:%s,url:%V rc:%d!",
                  r->request_start, &(r->uri), rc);

    return rc;
}


// 3
static void ngxrpc_inspect_application_interface(ngx_rpc_task_t* _this, void* p1)
{
     ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)p1;

     RpcChannel* cntl = new RpcChannel(inspect_ctx->r);

     cntl->task = _this;
     cntl->req  = new ngxrpc::inspect::Request();

     NgxChainBufferReader reader(_this->req_bufs);

     if(!cntl->req->ParseFromZeroCopyStream(&reader))
     {
         ngx_http_inspect_application_interface_done(NULL,
                                                     cntl,
                                                     NULL,
                                                     NULL,
                                                     NGX_HTTP_INTERNAL_SERVER_ERROR);
         return;
     }

     cntl->res = new ngxrpc::inspect::Response();
     cntl->done =  std::bind(ngx_http_inspect_application_interface_done,
                                     _this,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3,
                                     std::placeholders::_4);

     inspect_ctx->application_impl->interface(inspect_ctx->cntl,
                                 (ngxrpc::inspect::Request*)cntl->req,
                                 (ngxrpc::inspect::Response*)cntl->res,
                                 cntl->done);
}



void ngx_http_inspect_application_finish(void *ctx)
{
    ngx_rpc_task_t* task = (ngx_rpc_task_t*) ctx;
    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)task->closure.p1;

    ngx_http_request_t *r = inspect_ctx->r;

    static ngx_str_t type = ngx_string(" application/x-protobuf");

    r->headers_out.content_type = type;
    r->headers_out.status = task->response_states;
    r->headers_out.content_length_n = task->res_length;

    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;

    ngx_int_t rc = ngx_http_send_header(r);
    rc = ngx_http_output_filter(r, &task->res_bufs);
    ngx_http_finalize_request(r, rc);
}


static void ngx_http_inspect_application_interface_done(ngx_rpc_task_t *task,RpcChannel *channel,
                                                 const ::google::protobuf::Message* req,
                                                 ::google::protobuf::Message* res,
                                                 int result)
{
    task->response_states = result;
    NgxShmChainBufferWriter writer(task->res_bufs, task->pool);
    res->SerializeToZeroCopyStream(&writer);

    // clean the channel
    delete channel->req;
    delete channel->res;
    delete channel;


    ngx_rpc_notify_task(task->notify, ngx_http_inspect_application_finish, task);
}


static void ngxrpc_inspect_application_requeststatus(ngx_rpc_task_t* _this, void* p1)
{


}










