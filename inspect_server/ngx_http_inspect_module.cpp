// common header
#include "ngx_rpc_module/ngx_rpc_buffer.h"

// for rpc server header
#include "inspect_impl.h"
#include "ngx_rpc_server.h"
#include "ngx_rpc/ngx_rpc_queue.h"
#include "ngx_rpc/ngx_http_rpc.h"

// associate the http request

typedef struct {
    ngx_slab_pool_t *shpool;
    ngx_rpc_queue_t *queue;
    ngx_rpc_notify_t *notify;

    ngx_queue_t appendtask;
    ngx_shmtx_t shm_call_lock;

    ngx_http_request_t *r;

    // cache
    uint64_t timeout_ms;
} ngx_http_inspect_ctx_t;


void ngx_http_inspect_ctx_free(void* ctx)
{
    ngx_slab_free_locked(c->shpool, (ngx_http_inspect_ctx_t*)ctx);
}


typedef struct
{
     uint64_t timeout_ms;
     ngxrpc::inspect::ApplicationServer* application_impl;
     // other servers go here
} ngx_http_inspect_conf_t;



static void* ngx_http_inspect_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_inspect_conf_t *conf = (ngx_http_inspect_conf_t *)
            ngx_pcalloc(cf->pool, sizeof(ngx_http_inspect_conf_t));

    if(conf == NULL)
    {
        ERROR("ngx_http_inspect_conf_t create failed");
        return NULL;
    }

    conf->application_impl = NGX_CONF_UNSET_PTR;
    conf->timeout = NGX_CONF_UNSET_UINT;

    return conf;
}


static char *ngx_inspect_application_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_inspect_conf_t *c = (ngx_http_inspect_conf_t*)conf;

    c->application_impl = new ngxrpc::inspect::ApplicationServer();
    // Add some init

    ngx_http_core_loc_conf_t *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_inspect_http_handler;

    return NGX_OK;
}


static void ngx_inspect_process_exit(ngx_cycle_t* cycle)
{
    ngx_http_conf_ctx_t * ctx = (ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index];
    ngx_http_inspect_conf_t *c = (ngx_http_inspect_conf_t *) ctx ->loc_conf[module.ctx_index];

    delete c->application_impl;
}


/* Commands */
static ngx_command_t ngx_http_inspect_commands[] = {
    { ngx_string("inspect_application_init"),
      NGX_HTTP_LOC_CONF | NGX_CONF_ANY,
      ngx_inspect_application_init,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("inspect_timeout_ms"),
      NGX_HTTP_SRV_CONF | NGX_CONF_ANY,
      ngx_conf_set_msec_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_inspect_conf_t, timeout),
      NULL },

    ngx_null_command
};


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


#ifdef __cplusplus
extern "C" {
#endif

ngx_module_t ngx_http_inspect_module = {
    NGX_MODULE_V1,
    &ngx_http_inspect_module_ctx,              /* module context */
    ngx_http_rpc_module_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but some where called init module instead
    NULL,             /* init module */
    NULL,             /* init process */
    NULL,             /* init thread */
    NULL,             /* exit thread */
    ngx_inspect_process_exit,             /* exit process */
    NULL,             /* exit master */
    NGX_MODULE_V1_PADDING
};

#ifdef __cplusplus
}
#endif


static void  ngx_http_inspect_finish_request(void* cn)
{
    // 1 save res to chain buffer

    NgxRpcController *cntl = (NgxRpcController *)(((ngx_http_inspect_ctx_t* )cn)->cntl);

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

static ngx_rpc_task_t* ngx_http_inspect_application_interface_handler(ngx_http_inspect_ctx_t *ctx,ngx_rpc_task_t *pre, ngx_rpc_task_t *task)
{

    // 1 decode

    // 2 proces

    // 3 push terminotr


}
static ngx_rpc_task_t*  ngx_http_inspect_application_requeststatus_handler(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t *pre, ngx_rpc_task_t *task)
{

     // pop

     // done

     // push next , forward
}



static void ngx_http_inspect_application_subrequest_parent(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t *task)
{

     // dispath next
}



static void ngx_http_inspect_application_subrequest_done(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t *task)
{

    // cpy result

    // fowaord paraent
}


static void ngx_http_inspect_application_subrequest_begin(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t *task)
{

    // push sub task

    // forwaord
}


static void ngx_http_inspect_application_notify_done_handler(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t *pre, )
{

     // on notify

     // pop nofity task

     // set event write hanler

     //  result the parent handler ?
}



static void ngx_http_proc_proccess_task(void *c)
{
    ngx_http_inspect_ctx_t *ctx = (ngx_http_inspect_ctx_t *)c;

    ngx_rpc_task_t * task = NULL;
    ngx_rpc_task_t * pre = NULL;

    ngx_shmtx_lock(&ctx->shm_call_lock);
    pre = ngx_queue_next(ctx->appendtask);
    assert(task != NULL);
    ngx_queue_remove(task);
    ngx_shmtx_unlock(&ctx->shm_call_lock);


    ngx_rpc_task_t *next = task->process_hander(ctx, task);


    if(next)
    {
         ngx_shmtx_lock(&ctx->shm_call_lock);
         ngx_queue_insert_after(&ctx->appendtask, &next->next);
         //notify
         ngx_shmtx_unlock(&ctx->shm_call_lock);
    }
}



static void ngx_http_loc_proccess_task(void *c)
{
    ngx_http_inspect_ctx_t *ctx = (ngx_http_inspect_ctx_t *)c;

    ngx_rpc_task_t * task = NULL;

    ngx_shmtx_lock(&ctx->shm_call_lock);
    task = ngx_queue_next(ctx->appendtask);
    assert(task != NULL);
    ngx_queue_remove(task);
    ngx_shmtx_unlock(&ctx->shm_call_lock);

    ngx_rpc_task_t *next = task->process_hander(ctx, task);

    if(next)
    {
        ngx_http_inspect_dispatcher_task(ctx, next);
    }
}

static void ngx_http_inspect_dispatcher_task(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t* task){

    switch (task->type) {

    case PROCESS_IN_PROC:
        ngx_shmtx_lock(&ctx->shm_call_lock);
        ngx_queue_insert_after(&ctx->appendtask, &task->next);
        //notify
        ngx_rpc_push_task(ctx->queue, ctx);
        ngx_shmtx_unlock(&ctx->shm_call_lock);
        break;

    default:
        ngx_http_loc_proccess_task(ctx);
        break;
    }
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

    // 1 get a task
    ngx_http_inspect_conf_t *inspect_conf = (ngx_http_inspect_conf_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_inspect_module);
    ngx_http_inspect_ctx_t *ctx =
            ( ngx_http_inspect_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_inspect_module);


    // new process task
    ngx_rpc_task_t* task = (ngx_rpc_task_t*) ngx_slab_alloc_locked(ctx->shpool, sizeof(ngx_rpc_task_t));
     memset(task, 0, sizeof(task));

     task->init_time_ms = ngx_current_msec;
     task->time_out_ms  = ngx_current_msec + ctx->timeout_ms;

     //copy the request
     ngx_chain_t* req_chain = &task->req_bufs;

     for(ngx_chain_t* c= r->request_body->bufs; c; c=c->next )
     {
         int buf_size = c->buf->last - c->buf->pos;
         req_chain->buf = (ngx_buf_t*)ngx_slab_alloc_locked(ctx->shpool, sizeof(ngx_buf_t));
         memcpy(c->buf, req_chain->buf,sizeof(ngx_buf_t));
         req_chain->buf->pos = req_chain->buf->start = (u_char*) ngx_slab_alloc_locked(ctx->shpool,buf_size);
         memcpy(c->buf->pos, req_chain->buf->pos,buf_size);
         req_chain->next = (ngx_chain_t*)ngx_slab_alloc_locked(ctx->shpool, sizeof(ngx_chain_t));
         req_chain = req_chain->next;
         req_chain->next = NULL;
     }

     task->process_ctx = ctx;

    // router use hash
    if(0 == strncasecmp("/ngxrpc.inspect.Application.interface", r->uri.data, r->uri.len)
            || 0 == strncasecmp("/ngxrpc/inspect/Application.interface", r->uri.data, r->uri.len))
    {
        task->process_hander = ngx_http_inspect_application_interface_handler;
        task->object =inspect_conf->application_impl;
        task->type   = PROCESS_IN_PROC;
    }else  if(0 == strncasecmp("/ngxrpc.inspect.Application.requeststatus", r->uri.data, r->uri.len)
              || 0 == strncasecmp("/ngxrpc/inspect/Application/requeststatus", r->uri.data, r->uri.len))
    {
        task->process_hander = ngx_http_inspect_application_requeststatus_handler;
        task->object =inspect_conf->application_impl;
        task->type  = PROCESS_IN_SUBREQUEST;
    }else{
        ngx_http_rpc_destry_task(ctx,task);
        ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
        return;
    }

    // 2 dispatch task
    ngx_http_inspect_dispatcher_task(ctx, next);
}

static void ngx_http_inspect_http_handler(ngx_http_request_t *r)
{
    // 1 init the ctx
    ngx_http_inspect_ctx_t *inspect_ctx = (ngx_http_inspect_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_inspect_module);

    ngx_http_inspect_conf_t *inspect_conf = (ngx_http_inspect_conf_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_inspect_module);

    ngx_http_rpc_conf_t *rpc_conf = (ngx_http_rpc_conf_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_rpc_module);


    if (inspect_ctx == NULL)
    {
        inspect_ctx = (ngx_http_inspect_ctx_t *)
                ngx_slab_alloc_locked(rpc_conf->shpool, sizeof(ngx_http_inspect_ctx_t));

        if(ctx == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "ngx_palloc error size:%d",
                          sizeof(ngx_http_inspect_ctx_t));

            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        ngx_pool_cleanup_t *p1 = ngx_pool_cleanup_add(r->connection->pool, 0);
        p1->data = inspect_ctx;
        p1->handler = ngx_http_inspect_ctx_free;

        RpcChannel *cntl =  new RpcChannel(r);
        ngx_pool_cleanup_t *p2 = ngx_pool_cleanup_add(r->connection->pool, 0);
        p2->data = cntl;
        p2->handler = RpcChannel::destructor;

        inspect_ctx->timeout = inspect_conf->timeout;

        inspect_ctx->shpool = rpc_conf->shpool;
        inspect_ctx->queue  = rpc_conf->queue;
        inspect_ctx->notify = rpc_conf->

        inspect_ctx->state = NGX_RPC_CALL_INIT;

        ngx_http_set_ctx(r, inspect_ctx, ngx_http_inspect_module);
    }

    // 2 forward to the post handler
    ngx_int_t rc = ngx_http_read_client_request_body(r,
                         ngx_http_inspect_post_async_handler);

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        ngx_log_error(NGX_LOG_WARN,
                      r->connection->log,
                      0,
                      "Method:%s,url:%V rc:%d!",
                      r->request_start,
                      &(r->uri), rc);
    }

    return rc;
}











