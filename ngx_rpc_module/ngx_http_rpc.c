#include "ngx_http_rpc.h"

void ngx_http_rpc_ctx_destroy(void *p)
{
    ngx_http_rpc_ctx_t * ctx_ptr = p;
    ngx_slab_free(ctx_ptr->pool, p);

    if(ctx_ptr->task != NULL){
        ngx_http_rpc_task_destory(ctx_ptr->task);
    }

    ngx_log_error(NGX_LOG_DEBUG, ctx_ptr->method->log, 0,
                  "ngx_http_rpc_ctx_destroy ngx_http_rpc_ctx_t:%p", p);
}


/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {
    ngx_null_command
};

static ngx_int_t ngx_http_rpc_http_handler(ngx_http_request_t *r);

///
/// \brief ngx_http_rpc_postconfiguration
///        init all the method map and push the content handler
/// \param cf
/// \return
///
static ngx_int_t ngx_http_rpc_postconfiguration(ngx_conf_t* cf){

    ngx_http_rpc_conf_t *rpc_conf = (ngx_http_rpc_conf_t *)
            ngx_http_conf_get_module_main_conf(cf, ngx_http_rpc_module);


    ngx_array_t* eles = ngx_array_create(cf->pool, rpc_conf->method_array->nelts,
                                         sizeof(ngx_hash_key_t));

    unsigned int i = 0;

    for( i = 0; i< rpc_conf->method_array->nelts; ++i)
    {
        ngx_hash_key_t *key = (ngx_hash_key_t *)ngx_array_push(eles);

        method_conf_t *method = (method_conf_t *)(rpc_conf->method_array->elts) + i;

        key->key = method->name;
        key->key_hash = ngx_hash_key_lc(key->key.data, key->key.len);
        key->value = method;

        ngx_conf_log_error(NGX_LOG_INFO, cf, 0 ,
                           " ngx_http_rpc_add method %V:%p",&method->name, method);
    }


    ngx_hash_init_t hash_init = {
        NULL,
        &ngx_hash_key_lc,
        4,
        64,
        (char*)"ngx_http_rpc_methods",
        cf->pool,
        NULL
    };

    ngx_int_t ret = ngx_hash_init(&hash_init, (ngx_hash_key_t*)eles->elts, eles->nelts);

    if(ret != NGX_OK){
        ngx_array_destroy(eles);
        return NGX_ERROR;
    }

    rpc_conf->method_hash = hash_init.hash;
    ngx_array_destroy(eles);


    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_rpc_http_handler;

    return NGX_OK;
}

static void* ngx_http_rpc_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *)
            ngx_pcalloc(cf->pool,sizeof(ngx_http_rpc_conf_t));

    if(conf == NULL)
    {
        return NULL;
    }

    conf->proc_queue  = NULL;
    conf->notify      = NULL;
    conf->method_hash = NULL;
    // init the method
    conf->method_array = ngx_array_create(cf->pool, NGX_HTTP_RPC_METHOD_MAX,
                                          sizeof(method_conf_t));

    ngx_conf_log_error(NGX_LOG_INFO, cf, 0, "ngx_http_rpc_create_srv_conf :%p", conf);

    return conf;
}

/* Modules */
static ngx_http_module_t  ngx_http_rpc_module_ctx = {
    NULL,                                   /* preconfiguration */
    ngx_http_rpc_postconfiguration,         /* postconfiguration */
    ngx_http_rpc_create_main_conf,          /* create main configuration */
    NULL,            /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle);
static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle);

ngx_module_t  ngx_http_rpc_module = {
    NGX_MODULE_V1,
    &ngx_http_rpc_module_ctx,              /* module context */
    ngx_http_rpc_module_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but somewhere called init module instead
    NULL,                                  /* init module */
    ngx_http_rpc_init_process,             /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_http_rpc_exit_process,             /* exit process */
    NULL,
    NGX_MODULE_V1_PADDING
};



///
#define ngx_http_cycle_get_module_srv_conf(cycle, module)            \
    (cycle->conf_ctx[ngx_http_module.index] ?                        \
    ((ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index]) \
    ->srv_conf[module.ctx_index] : NULL)


static void ngx_http_rpc_process_notify_task(void *ctx);

static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle)
{

    ngx_http_rpc_conf_t *rpc_conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_proc_rpc_conf_t *proc_conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(rpc_conf == NULL || proc_conf == NULL)
    {
        ngx_log_error(NGX_LOG_INFO, cycle->log, 0,
                      "Neither ngx_http_rpc_module nor ngx_proc_rpc_module was add to tengine");
        return NGX_OK;
    }

    rpc_conf->proc_queue = proc_conf->queue;

    rpc_conf->notify = ngx_rpc_queue_add_current_producer(rpc_conf->proc_queue, rpc_conf);

    rpc_conf->notify->read_hanlder  = ngx_http_rpc_process_notify_task;
    rpc_conf->notify->write_hanlder = ngx_http_rpc_process_notify_task;

    ngx_log_error(NGX_LOG_WARN, cycle->log, 0,
                  "ngx_http_rpc_init_process rpc_conf:%p queue:%p notify eventfd:%d",
                  rpc_conf, rpc_conf->proc_queue, rpc_conf->notify->event_fd);

    return NGX_OK;
}

// remove frome
static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_log_error(NGX_LOG_WARN, cycle->log, 0,
                  "ngx_http_rpc_exit_process conf:%p unregister notify eventfd:%d",
                  conf, conf->notify->event_fd);

    ngx_rpc_notify_unregister(conf->notify);
}



// for content handler
static void ngx_http_rpc_post_async_handler(ngx_http_request_t *r)
{
    if(r->request_body == NULL
            || r->request_body->bufs == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "url:%V Request body is NULL", &r->uri);

        ngx_http_finalize_request(r, NGX_HTTP_LENGTH_REQUIRED);
        return;
    }

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    if(rpc_ctx->task == NULL)
    {
        rpc_ctx->task = ngx_http_rpc_task_create(rpc_ctx->pool,
                                                 r->connection->pool,
                                                 rpc_ctx->method->log,
                                                 rpc_ctx->method->exec_in_nginx);
    }else{
        rpc_ctx->task = ngx_http_rpc_task_reset(rpc_ctx->task,
                                                rpc_ctx->pool,
                                                r->connection->pool,
                                                rpc_ctx->method->log,
                                                rpc_ctx->method->exec_in_nginx);
    }

    // this process eventfd
    rpc_ctx->task->done_notify = rpc_ctx->rpc_conf->notify;
    rpc_ctx->task->proc_notify = rpc_ctx->rpc_conf->notify;

    // copy the request bufs
    ngx_http_rpc_task_set_bufs(rpc_ctx->task->pool, &rpc_ctx->task->req_bufs, r->request_body->bufs);
    rpc_ctx->task->req_length = r->headers_in.content_length_n;

    // processor which run in the proc process.
    rpc_ctx->task->closure.handler = rpc_ctx->method->handler;
    rpc_ctx->task->closure.p1  =  rpc_ctx;

    //
    if(rpc_ctx->task->exec_in_nginx)
    {
        task->closure.handler(task, task->closure.p1);

    }else{

        rpc_ctx->task->proc_notify =
                ngx_rpc_queue_get_idle(rpc_ctx->rpc_conf->proc_queue);

        if(rpc_ctx->task->proc_notify == NULL)
        {
            rpc_ctx->task->response_states = NGX_HTTP_GATEWAY_TIME_OUT;
            rpc_ctx->task->res_length = 0;
            ngx_http_rpc_request_finish(rpc_ctx->task, r);
        }

        ngx_rpc_notify_push_task(rpc_ctx->task->proc_notify, rpc_ctx->task);
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
static ngx_int_t ngx_http_rpc_http_handler(ngx_http_request_t *r)
{
    // 1 init the ctc

    ngx_http_rpc_conf_t *rpc_conf = (ngx_http_rpc_conf_t *)
            ngx_http_get_module_main_conf(r, ngx_http_rpc_module);


    if(rpc_conf == NULL || rpc_conf->proc_queue == NULL)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "Method:%s,url:%V rpc_conf:%p !",
                      r->request_start, &(r->uri), rpc_conf);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }


    // 2 find the hanlder if not found termial the request
    method_conf_t *mth = (method_conf_t*) ngx_hash_find(
                rpc_conf->method_hash,
                ngx_hash_key_lc(r->uri.data, r->uri.len),
                r->uri.data, r->uri.len);

    if(mth == NULL)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "url:%V  not found ngx rpc content handler",
                      &(r->uri));

        return NGX_DECLINED;
    }

    // 1 when a new request call in ,init the ctx
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    if(rpc_ctx == NULL)
    {
        //do something
        rpc_ctx = (ngx_http_rpc_ctx_t *) ngx_slab_alloc(
                    rpc_conf->proc_queue->pool, sizeof(ngx_http_rpc_ctx_t));

        if(rpc_ctx == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "ngx_palloc error size:%d",
                          sizeof(ngx_http_rpc_ctx_t));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        rpc_ctx->rpc_conf = rpc_conf;
        rpc_ctx->pool = rpc_conf->proc_queue->pool;
        rpc_ctx->r = r;
        rpc_ctx->method = mth;
        rpc_ctx->task = NULL;


        // the destructor of ngx_http_inspect_ctx_t
        ngx_pool_cleanup_t *p1 = ngx_pool_cleanup_add(r->connection->pool, 0);
        p1->data = rpc_ctx;
        p1->handler = ngx_http_rpc_ctx_destroy;

        ngx_http_set_ctx(r, rpc_ctx, ngx_http_rpc_module);
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                      "init the rpc_ctx:%p uri:%V", rpc_ctx, &(r->uri));
    }

    // 2 forward to the post handler
    ngx_int_t rc = ngx_http_read_client_request_body(r,
                                               ngx_http_rpc_post_async_handler);

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "url:%V forward to ngx rpc method:%V rc:%d",
                  &r->method_name, &mth->name, rc);

    // need ngx_http_rpc_post_async_handler to process the request
    return NGX_AGAIN;
}



/// for nofity process

static void ngx_http_rpc_process_notify_task(void *ctx)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *) ctx;

    ngx_queue_t* pending = NULL;

    while(ngx_rpc_notify_pop_task(conf->notify, &pending) == NGX_OK)
    {
        ngx_rpc_task_t * task = ngx_queue_data(pending, ngx_rpc_task_t, node);

        ngx_log_debug(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0 ,
                      "ngx_http_rpc_process_notify_task rpc_conf:%p, notify_eventfd:%d task:%p done_eventfd:%d ",
                      conf, conf->notify->event_fd, task, task->done_notify->event_fd);

        task->finish.handler(task, task->finish.p1);
    }
}

/// for finish task
void ngx_http_rpc_request_finish(ngx_rpc_task_t* _this, void *ctx)
{
    ngx_rpc_task_t* task = _this;
    ngx_http_request_t *r = ctx;

    static ngx_str_t type = ngx_string(" application/x-protobuf");

    r->headers_out.content_type = type;
    r->headers_out.status = task->response_states;


    if(task->res_length >= 0)
    {
        r->headers_out.content_length_n = task->res_length;
        r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;
    }

    int rc = ngx_http_send_header(r);

    if(rc == NGX_OK && task->res_length > 0)
    {
        //find last
        rc = ngx_http_output_filter(r, &task->res_bufs);
    }

    ngx_log_error(NGX_LOG_INFO, task->log, 0,
                  "ngx_http_inspect_application_finish task:%p status:%d size:%d, rc:%d",
                  task, task->response_states, task->res_length, rc );

    ngx_http_finalize_request(r, rc);
}



/// for sub request task
static ngx_int_t
ngx_http_rpc_subrequest_done_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_rpc_task_t* task = (ngx_rpc_task_t*)data;

    // came from the parent
    if(rc == NGX_AGAIN)
    {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                      "ngx_http_rpc_subrequest_done_handler NGX_AGAIN task:%p status:%d nofity eventfd:%d rc:%d",
                      task, r->headers_out.status, task->proc_notify->event_fd, rc);

        return NGX_OK;
    }

    // ngx_http_request_t *pr = r->parent;
    task->response_states = r->headers_out.status;

    if(task->response_states == NGX_HTTP_OK )
    {
        ngx_http_rpc_task_set_bufs(task->pool, &task->req_bufs,
                 r->upstream ? r->upstream->out_bufs : r->postponed->out);
    }

    if(task->exec_in_nginx)
    {
        task->closure.handler(task, task->closure.p1);
    }else{
        // send to proc process
        ngx_rpc_notify_push_task(task->proc_notify, &task->node);
    }

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "ngx_http_rpc_subrequest_done_handler task:%p status:%d nofity eventfd:%d",
                  task, r->headers_out.status, task->proc_notify->event_fd);

    return NGX_OK;
}

int ngx_http_header_modify_content_length(ngx_http_request_t *r, ngx_int_t value)
{
    r->headers_in.content_length_n = value;

    ngx_list_part_t *part =  &r->headers_in.headers.part;
    ngx_table_elt_t *header = (ngx_table_elt_t *)part->elts;
    unsigned int i = 0;
    for( i = 0; /* void */ ; i++)
    {
        if(i >= part->nelts)
        {
            if( part->next  == NULL)
            {
                break;
            }

            part = part->next;
            header = (ngx_table_elt_t *)part->elts;
            i = 0;
        }

        if(header[i].hash == 0)
        {
            continue;
        }

        if(0 == ngx_strncasecmp(header[i].key.data,
                                (u_char*)"Content-Length:",
                                header[i].key.len))
        {

            header[i].value.data = (u_char *)ngx_palloc(r->pool, 32);
            header[i].value.len = 32;

            snprintf((char*)header[i].value.data,
                     32, "%ld", value);
            r->headers_in.content_length->value =  header[i].value;

            return NGX_OK;
        }
    }
    return NGX_ERROR;
}

void ngx_http_rpc_request_foward(ngx_rpc_task_t* _this, void *ctx)
{

    ngx_rpc_task_t* task = _this;
    ngx_http_request_t *r = ctx;

    ngx_http_post_subrequest_t *psr =
            ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));

    psr->handler = ngx_http_rpc_subrequest_done_handler;
    psr->data = task;

    r->request_body->bufs = &task->req_bufs;

    ngx_http_header_modify_content_length(r, task->res_length);

    ngx_str_t forward = { strlen((char*)task->path), task->path};

    ngx_http_request_t *sr;

    ngx_int_t rc = ngx_http_subrequest(r, &forward, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  " start sub request %V task:%p content length:%d rc:%d",
                  &forward, task, task->res_length, rc);

    ngx_http_run_posted_requests(r->connection);
}


