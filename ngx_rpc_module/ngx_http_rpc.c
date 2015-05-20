#include "ngx_http_rpc.h"

void ngx_http_rpc_ctx_destroy(void *p)
{

    ngx_http_rpc_ctx_t * ctx_ptr = p;
    if(ctx_ptr->task != NULL){
        ngx_http_rpc_task_destory(ctx_ptr->task);
    }

    if(!ctx_ptr->method->exec_in_nginx)
    {
        ngx_slab_free(ctx_ptr->pool, p);
    }

    ngx_log_error(NGX_LOG_DEBUG, ctx_ptr->method->log, 0,
                  "ngx_http_rpc_ctx_destroy ngx_http_rpc_ctx_t:%p", p);
}
static ngx_conf_bitmask_t  ngx_http_rpc_next_upstream_masks[] = {
    { ngx_string("error"), NGX_HTTP_UPSTREAM_FT_ERROR },
    { ngx_string("timeout"), NGX_HTTP_UPSTREAM_FT_TIMEOUT },
    { ngx_string("invalid_response"), NGX_HTTP_UPSTREAM_FT_INVALID_HEADER },
    { ngx_string("not_found"), NGX_HTTP_UPSTREAM_FT_HTTP_404 },
    { ngx_string("off"), NGX_HTTP_UPSTREAM_FT_OFF },
    { ngx_null_string, 0 }
};


/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {

    { ngx_string("ngx_rpc_bind"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_upstream_bind_set_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rpc_loc_conf_t, upstream.local),
      NULL },

    { ngx_string("ngx_rpc_connect_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rpc_loc_conf_t, upstream.connect_timeout),
      NULL },

    { ngx_string("ngx_rpc_send_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rpc_loc_conf_t, upstream.send_timeout),
      NULL },

    { ngx_string("ngx_rpc_buffer_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rpc_loc_conf_t, upstream.buffer_size),
      NULL },

    { ngx_string("ngx_rpc_read_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rpc_loc_conf_t, upstream.read_timeout),
      NULL },

    { ngx_string("ngx_rpc_next_upstream"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rpc_loc_conf_t, upstream.next_upstream),
      &ngx_http_rpc_next_upstream_masks },


    { ngx_string("ngx_rpc_upstream_tries"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rpc_loc_conf_t, upstream.upstream_tries),
      NULL },

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

    ngx_http_rpc_main_conf_t *rpc_conf = (ngx_http_rpc_main_conf_t *)
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

        ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0 ,
                           " ngx_http_rpc_add method:%V, hash:%u",&method->name, key->key_hash);
    }


    ngx_hash_init_t hash_init = {
        NULL,
        &ngx_hash_key_lc,
        64,
        256,
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
    ngx_http_rpc_main_conf_t *conf = (ngx_http_rpc_main_conf_t *)
            ngx_pcalloc(cf->pool,sizeof(ngx_http_rpc_main_conf_t));

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

static void* ngx_http_rpc_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_rpc_loc_conf_t *conf = (ngx_http_rpc_loc_conf_t *)
            ngx_pcalloc(cf->pool, sizeof(ngx_http_rpc_loc_conf_t));

    if(conf == NULL)
    {
        return NULL;
    }
    conf->upstream.local = NGX_CONF_UNSET_PTR;
    conf->upstream.connect_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.send_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.read_timeout = NGX_CONF_UNSET_MSEC;

    conf->upstream.buffer_size = NGX_CONF_UNSET_SIZE;

    conf->upstream.upstream_tries = NGX_CONF_UNSET_UINT;

    /* the hardcoded values */
    conf->upstream.cyclic_temp_file = 0;
    conf->upstream.buffering = 0;
    conf->upstream.ignore_client_abort = 0;
    conf->upstream.send_lowat = 0;
    conf->upstream.bufs.num = 0;
    conf->upstream.busy_buffers_size = 0;
    conf->upstream.max_temp_file_size = 0;
    conf->upstream.temp_file_write_size = 0;
    conf->upstream.intercept_errors = 1;
    conf->upstream.intercept_404 = 1;
    conf->upstream.pass_request_headers = 0;
    conf->upstream.pass_request_body = 0;

    conf->upstream.hide_headers = NGX_CONF_UNSET_PTR;
    conf->upstream.pass_headers = NGX_CONF_UNSET_PTR;
    return conf;
}


static char *
ngx_http_rpc_proxy_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{

    ngx_http_rpc_loc_conf_t *prev = parent;
    ngx_http_rpc_loc_conf_t *conf = child;

    ngx_conf_merge_ptr_value(conf->upstream.local,
                             prev->upstream.local, NULL);

    ngx_conf_merge_ptr_value(conf->upstream.local,
                             prev->upstream.local, NULL);

    ngx_conf_merge_msec_value(conf->upstream.connect_timeout,
                              prev->upstream.connect_timeout, 60000);

    ngx_conf_merge_msec_value(conf->upstream.send_timeout,
                              prev->upstream.send_timeout, 60000);

    ngx_conf_merge_msec_value(conf->upstream.read_timeout,
                              prev->upstream.read_timeout, 60000);

    ngx_conf_merge_size_value(conf->upstream.buffer_size,
                              prev->upstream.buffer_size,
                              (size_t) ngx_pagesize);

    ngx_conf_merge_bitmask_value(conf->upstream.next_upstream,
                                 prev->upstream.next_upstream,
                                 (NGX_CONF_BITMASK_SET
                                  |NGX_HTTP_UPSTREAM_FT_ERROR
                                  |NGX_HTTP_UPSTREAM_FT_TIMEOUT));

    if (conf->upstream.next_upstream & NGX_HTTP_UPSTREAM_FT_OFF) {
        conf->upstream.next_upstream = NGX_CONF_BITMASK_SET
                |NGX_HTTP_UPSTREAM_FT_OFF;
    }

    ngx_conf_merge_uint_value(conf->upstream.upstream_tries,
                              prev->upstream.upstream_tries,
                              NGX_CONF_UNSET_UINT);

    if (conf->upstream.upstream == NULL) {
        conf->upstream.upstream = prev->upstream.upstream;
    }

    static ngx_str_t  ngx_http_rpc_proxy_hide_headers[] = {
        ngx_string("X-Pad"),
        ngx_string("X-Accel-Expires"),
        ngx_string("X-Accel-Redirect"),
        ngx_string("X-Accel-Limit-Rate"),
        ngx_string("X-Accel-Buffering"),
        ngx_string("X-Accel-Charset"),
        ngx_null_string
    };

    ngx_hash_init_t hash;
    hash.max_size = 16;
    hash.bucket_size = 64;
    hash.name = "ngx_http_rpc_proxy_hide_headers_hash";

    if (ngx_http_upstream_hide_headers_hash(cf,
           &conf->upstream,
           &prev->upstream,
           ngx_http_rpc_proxy_hide_headers, &hash) != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

/* Modules */
static ngx_http_module_t  ngx_http_rpc_module_ctx = {
    NULL,                                   /* preconfiguration */
    ngx_http_rpc_postconfiguration,         /* postconfiguration */
    ngx_http_rpc_create_main_conf,          /* create main configuration */
    NULL,            /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    ngx_http_rpc_create_loc_conf,                                  /* create location configuration */
    ngx_http_rpc_proxy_merge_conf                                   /* merge location configuration */
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

    ngx_http_rpc_main_conf_t *rpc_conf =
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
    ngx_http_rpc_main_conf_t *conf =
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
        rpc_ctx->task = ngx_http_create_rpc_task(rpc_ctx->pool,
                                                 r->connection->pool,
                                                 r->connection->log,
                                                 rpc_ctx->method->exec_in_nginx);
    }else{
        rpc_ctx->task = ngx_http_rpc_task_reset(rpc_ctx->task,
                                                rpc_ctx->pool,
                                                r->connection->pool,
                                                r->connection->log);
    }

    // this process eventfd
    rpc_ctx->task->done_notify = rpc_ctx->rpc_conf->notify;
    rpc_ctx->task->proc_notify = rpc_ctx->rpc_conf->notify;

    // copy the request bufs
    ngx_http_rpc_task_set_bufs(rpc_ctx->task, &rpc_ctx->task->req_bufs, r->request_body->bufs);

    rpc_ctx->task->req_length = r->headers_in.content_length_n;

    // processor which run in the proc process.
    rpc_ctx->task->closure.handler = rpc_ctx->method->handler;
    rpc_ctx->task->closure.p1  =  rpc_ctx;

    //
    if(rpc_ctx->task->exec_in_nginx)
    {
        rpc_ctx->task->closure.handler(rpc_ctx->task, rpc_ctx->task->closure.p1);

    }else{

        rpc_ctx->task->proc_notify =
                ngx_rpc_queue_get_idle(rpc_ctx->rpc_conf->proc_queue);

        if(rpc_ctx->task->proc_notify == NULL)
        {
            rpc_ctx->task->response_states = NGX_HTTP_GATEWAY_TIME_OUT;
            rpc_ctx->task->res_length = 0;
            ngx_http_rpc_request_finish(rpc_ctx->task, r);
        }
        ngx_rpc_notify_push_task(rpc_ctx->task->proc_notify, &(rpc_ctx->task->node));
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

    ngx_http_rpc_main_conf_t *rpc_conf = (ngx_http_rpc_main_conf_t *)
            ngx_http_get_module_main_conf(r, ngx_http_rpc_module);


    if(rpc_conf == NULL || rpc_conf->proc_queue == NULL)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "Method:%s,url:%V rpc_conf:%p !",
                      r->request_start, &(r->uri), rpc_conf);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }


    // 2 find the hanlder if not found termial the request

    ngx_uint_t key = ngx_hash_strlow(r->uri.data, r->uri.data, r->uri.len);

    method_conf_t *mth = (method_conf_t*) ngx_hash_find(
                rpc_conf->method_hash,
                key,
                r->uri.data, r->uri.len);

    if(mth == NULL)
    {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "url:%V  hash:%u not found ngx rpc content handler ",
                      &(r->uri), ngx_hash_key_lc(r->uri.data, r->uri.len));

        return NGX_DECLINED;
    }

    // 1 when a new request call in ,init the ctx
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    if(rpc_ctx == NULL)
    {
        //do something

        if(mth->exec_in_nginx)
        {
            rpc_ctx = (ngx_http_rpc_ctx_t *) ngx_palloc(r->pool, sizeof(ngx_http_rpc_ctx_t));
        }else{
            rpc_ctx = (ngx_http_rpc_ctx_t *) ngx_slab_alloc(
                        rpc_conf->proc_queue->pool, sizeof(ngx_http_rpc_ctx_t));
        }

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

        // push back
        ngx_log_t ** ptr = &r->connection->log->next;
        for(; *ptr; ptr = &(*ptr)->next);
        *ptr = rpc_ctx->method->log;
    }


    // 2 forward to the post handler
    ngx_int_t rc = ngx_http_read_client_request_body(r,
                                                     ngx_http_rpc_post_async_handler);

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "url:%V process by ngx rpc method:%V rc:%d",
                  &r->uri, &mth->name, rc);

    // need ngx_http_rpc_post_async_handler to process the request
    return NGX_AGAIN;
}



/// for nofity process

static void ngx_http_rpc_process_notify_task(void *ctx)
{
    ngx_http_rpc_main_conf_t *conf = (ngx_http_rpc_main_conf_t *) ctx;

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


    if(task->res_length <= 0 && task->response_states == NGX_HTTP_OK)
    {
        task->response_states = NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->headers_out.content_type = type;
    r->headers_out.status = task->response_states;

    if(task->response_states != NGX_HTTP_OK)
    {
        ngx_log_error(NGX_LOG_ERR, task->log, 0,
                      "ngx_http_rpc_request_finish task:%p status:%d size:%d",
                      task, task->response_states, task->res_length );

        //r->header_sent = 0;
        //r->header_only = 1;
        ngx_http_finalize_request(r, task->response_states);
        return;
    }


    r->headers_out.content_length_n = task->res_length;
    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;

    int rc = ngx_http_send_header(r);

    if(rc == NGX_OK && task->res_length > 0)
    {
        rc = ngx_http_output_filter(r, &task->res_bufs);
    }else{
        r->header_only = 1;
        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_log_error(NGX_LOG_INFO, task->log, 0,
                  "ngx_http_rpc_request_finish task:%p status:%d size:%d, rc:%d",
                  task, task->response_states, task->res_length, rc );

    ngx_http_finalize_request(r, task->response_states);
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
        ngx_http_rpc_task_set_bufs(task, &task->req_bufs,
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




static ngx_int_t ngx_http_rpc_upstream_create_request(ngx_http_request_t *r)
{

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    //
    r->upstream->request_bufs = ngx_alloc_chain_link(r->pool);

    // 256 is max the header size;
    ngx_buf_t *buf = ngx_create_temp_buf(r->pool, 256);
    r->upstream->request_bufs->buf  = buf;
    r->upstream->request_bufs->next =&rpc_ctx->task->req_bufs;

    buf->last = ngx_snprintf(buf->last, 256,
                "POST %s HTTP/1.1\r\nContent-Type: application/x-protobuf\r\nContent-Length: %d\r\nHost: localhost\r\n\r\n",
                 rpc_ctx->task->path, rpc_ctx->task->req_length);

    r->upstream->request_sent = 0;
    r->upstream->header_sent = 0;
    r->header_hash = 1;

    // clear first
    memset(&rpc_ctx->status, 0, sizeof(rpc_ctx->status));

    ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
                  "ngx_http_rpc_upstream_create_request with task:%p body_size:%d",
                  rpc_ctx->task, rpc_ctx->task->req_length);



    return NGX_OK;
}


static ngx_int_t ngx_http_rpc_upstream_reinit_request(ngx_http_request_t *r)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log,
                  0, "ngx_http_rpc_upstream_reinit_request with task:%p body_size:%d",
                  rpc_ctx->task, rpc_ctx->task->req_length);

    // clear first
    memset(&rpc_ctx->status, 0, sizeof(rpc_ctx->status));

    return NGX_OK;
}



static ngx_int_t ngx_http_rpc_upstream_process_header(ngx_http_request_t *r)
{
    ngx_int_t rc;
    ngx_table_elt_t *h;
    ngx_http_upstream_header_t *header;
    ngx_http_upstream_main_conf_t *umcf;

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);


    ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log,
                  0, "ngx_http_rpc_upstream_process_header task:%p ", rpc_ctx->task);

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

    for(;;)
    {
        rc = ngx_http_parse_header_line(r, &r->upstream->buffer, 1);


        if(rc == NGX_OK)
        {
            h = ngx_list_push(&r->upstream->headers_in.headers);

            if(h == NULL)
            {
                return NGX_ERROR;
            }


            h->hash = r->header_hash;

            h->key.len = r->header_name_end - r->header_name_start;
            h->value.len = r->header_end - r->header_start;
            h->key.data = ngx_pnalloc(r->pool, h->key.len + 1 + h->value.len + 1 + h->key.len);

            if(h->key.data == NULL)
            {
                return NGX_ERROR;
            }

            h->value.data = h->key.data + h->key.len + 1;
            h->lowcase_key = h->key.data + h->key.len + 1 + h->value.len + 1;

            ngx_memcpy(h->key.data, r->header_name_start, h->key.len);
            h->key.data[h->key.len] = '\0';

            ngx_memcpy(h->value.data, r->header_start,h->value.len);
            h->value.data[h->value.len] = '\0';

            if (h->key.len == r->lowcase_index)
            {
                ngx_memcpy(h->lowcase_key, r->lowcase_header, h->key.len);
            }else{

                ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
            }

            header = ngx_hash_find(&umcf->headers_in_hash, h->hash, h->lowcase_key, h->key.len);


            //ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            //              "ngx_http_parse_header_line :rc:%d, lowcase_key:%s key:%V value:%V header name:%V"
            //              , rc , h->lowcase_key, &h->key, &h->value, &header->name);

            /**set the header correspond member in the r */

            if(header && header->handler(r, h, header->offset) != NGX_OK)
            {
                return NGX_ERROR;
            }

            continue;
        }

        if(rc == NGX_HTTP_PARSE_HEADER_DONE)
        {
            if(r->upstream->headers_in.server == NULL)
            {
                h = ngx_list_push(&r->upstream->headers_in.headers);

                if(h == NULL)
                    return NGX_ERROR;

                h->hash = ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash('s', 'e'), 'r'),'v'), 'e'), 'r');

                ngx_str_set(&h->key, "Server");
                ngx_str_null(&h->value);

                h->lowcase_key = (u_char *)"server";
            }

            if(r->upstream->headers_in.date == NULL)
            {
                h =ngx_list_push(&r->upstream->headers_in.headers);

                if(h == NULL)
                    return NGX_ERROR;

                h->hash =
                        ngx_hash(ngx_hash(ngx_hash('d', 'a'), 't'),'e');

                ngx_str_set(&h->key, "Date");
                ngx_str_null(&h->value);

                h->lowcase_key = (u_char *)"date";
            }

            return NGX_OK;
        }

        return rc == NGX_AGAIN ? NGX_AGAIN :NGX_HTTP_UPSTREAM_INVALID_HEADER;
    }
}

static ngx_int_t ngx_http_rpc_upstream_process_status_line(ngx_http_request_t *r)
{

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);
    ngx_http_upstream_t *u = r->upstream;

    ngx_int_t rc = ngx_http_parse_status_line(r, &u->buffer, &rpc_ctx->status);

    if(rc == NGX_AGAIN)
    {
        return rc;
    }

    if(rc == NGX_ERROR)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "upstream sent no vaild HTTP/1.0 header rc:%d", rc);

        return NGX_ERROR;
    }

    if(u->state)
    {
        u->state->status = rpc_ctx->status.code;
    }

    u->headers_in.status_n = rpc_ctx->status.code;

    size_t len = rpc_ctx->status.end - rpc_ctx->status.start;

    u->headers_in.status_line.len = len;
    u->headers_in.status_line.data = ngx_pnalloc(r->pool, len);

    if(u->headers_in.status_line.data == NULL)
        return NGX_ERROR;

    ngx_memcpy(u->headers_in.status_line.data, rpc_ctx->status.start, len);


    ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log,
                  0, "ngx_http_rpc_upstream_process_status_line with task:%p rc:%d status:%d status_line:%V",
                  rpc_ctx->task, rc, rpc_ctx->status.code, &u->headers_in.status_line);

    u->process_header =  ngx_http_rpc_upstream_process_header;

    return ngx_http_rpc_upstream_process_header(r);
}

static void  ngx_http_rpc_upstream_abort_request(ngx_http_request_t *r)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    ngx_log_debug(NGX_LOG_ERR, r->connection->log,
                  0, "ngx_http_rpc_upstream_abort_request with task:%p status:%d",
                  rpc_ctx->task, rpc_ctx->status.code);

    rpc_ctx->task->response_states = NGX_HTTP_SERVICE_UNAVAILABLE;

    rpc_ctx->task->res_length = 0;

    if(rpc_ctx->task->exec_in_nginx)
    {
        rpc_ctx->task->closure.handler(rpc_ctx->task, rpc_ctx->task->closure.p1);
    }else{
        ngx_rpc_notify_push_task(rpc_ctx->task->proc_notify, &rpc_ctx->task->node);
    }

}

static void  ngx_http_rpc_upstream_finalize_request(ngx_http_request_t *r,
                                                    ngx_int_t rc){

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    rpc_ctx->task->response_states  = rpc_ctx->status.code;

    ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
                  "ngx_http_rpc_upstream_finalize_request with task:%p body_size:%d rc:%d status:%d",
                  rpc_ctx->task, rpc_ctx->task->res_length, rc, rpc_ctx->status.code);

    if(rpc_ctx->task->exec_in_nginx)
    {
        rpc_ctx->task->closure.handler(rpc_ctx->task, rpc_ctx->task->closure.p1);
    }else{
        ngx_rpc_notify_push_task(rpc_ctx->task->proc_notify, &rpc_ctx->task->node);
    }
}

// to store the buf to task
static ngx_int_t ngx_http_rpc_upstream_input_filter_init(void *data)
{
    ngx_http_request_t *r = data;

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    ngx_http_upstream_t  *u = r->upstream;


    rpc_ctx->task->res_length = 0;
    rpc_ctx->task->response_states =u->headers_in.status_n;
    ngx_http_rpc_task_set_bufs(rpc_ctx->task, &rpc_ctx->task->res_bufs, NULL);



    u->length = u->headers_in.status_n == NGX_HTTP_OK ?
                   u->headers_in.content_length_n : 0;

    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                  "ngx_http_rpc_upstream_input_filter_init response_states:%d content_length_n:%d",
                  rpc_ctx->task->response_states, u->headers_in.content_length_n);

    return NGX_OK;
}


static ngx_int_t ngx_http_rpc_upstream_non_buffered_copy_filter(void *data, ssize_t bytes)
{
    ngx_http_request_t *r = data;

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);
    ngx_rpc_task_t* task = rpc_ctx->task;

    ngx_http_upstream_t  *u = r->upstream;
    ngx_buf_t            *b = &u->buffer;
    ngx_chain_t          *cl, **ll;


    if(data == NULL || bytes <= 0)
    {
        ngx_log_error(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
                      "u->buffer empty ngx_http_request_t:%p bytes:%d",
                      data, bytes);
        return NGX_OK;
    }

    for (cl = &task->res_bufs, ll = &(task->res_bufs.next); cl && cl->buf ; cl = cl->next)
    {
        cl->buf->last_buf = 0;
        ll = &cl->next;
    }

    //
    if(task->exec_in_nginx)
    {
        if(cl == NULL)
        {
            *ll = ngx_palloc(r->pool, sizeof(ngx_chain_t));
            (*ll)->next = NULL;
             cl = *ll;
        }

        cl->buf = ngx_create_temp_buf(r->pool, bytes);
        cl->buf->last = cl->buf->pos + bytes;
        cl->buf->last_buf = 1;

    }else{

        if(cl == NULL)
        {
            *ll = ngx_slab_alloc(task->share_pool, sizeof(ngx_chain_t));
            (*ll)->next = NULL;
             cl = *ll;
        }

        cl->buf = ngx_slab_alloc(task->share_pool, sizeof(ngx_buf_t) + bytes);

        cl->buf->start = cl->buf->pos = ((u_char*)cl->buf) + sizeof(ngx_buf_t);
        cl->buf->end   = cl->buf->last = cl->buf->pos + bytes;

        cl->buf->temporary = 1;
        cl->buf->memory    = 1;
        cl->buf->file      = 0;
        cl->buf->last_buf  = 1;
    }

    memcpy(cl->buf->pos , b->pos, bytes);
    b->last += bytes;

    task->res_length += bytes;

    u->length = u->length > 0 ? (u->length - bytes) : -1 ;

    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                  "u->buffer bytes:%d task:%p task_length:%d upstream length:%d",
                  bytes, task, task->res_length, u->length);

    return NGX_OK;
}


// for upstream
void ngx_http_rpc_start_upsteam(ngx_rpc_task_t* _this, void *ctx){

    // init upstream
    ngx_http_request_t *r = (ngx_http_request_t *)ctx;


   // ngx_http_rpc_main_conf_t *rpc_conf = (ngx_http_rpc_main_conf_t *)
   //         ngx_http_get_module_main_conf(r, ngx_http_rpc_module);

    ngx_http_rpc_loc_conf_t *rpc_loc_conf = (ngx_http_rpc_loc_conf_t *)
            ngx_http_get_module_loc_conf(r, ngx_http_rpc_module);

    //only one upstream ?
    if (ngx_http_upstream_create(r) != NGX_OK) {

        _this->response_states = NGX_HTTP_SERVICE_UNAVAILABLE;

        if(_this->exec_in_nginx)
        {
            _this->finish.handler(_this, _this->finish.p1);
        }else{
            ngx_rpc_notify_push_task(_this->done_notify, &_this->node);
        }

        return;
    }


    ngx_http_upstream_t *u = r->upstream;

    u->schema.data = (u_char*)"http://";
    u->schema.len  = sizeof("http://");

    u->output.tag = (ngx_buf_tag_t) &ngx_http_rpc_module;

    u->conf = &rpc_loc_conf->upstream;

    // only support the location by upstream
    ngx_http_upstream_main_conf_t  *umcf =
            ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

    ngx_http_upstream_srv_conf_t **uscfp = umcf->upstreams.elts;

    size_t len = strlen((char*)_this->path);
    ngx_uint_t i = 0;
    u->conf->upstream = NULL;

    for (i = 0; i < umcf->upstreams.nelts; i++) {
        int min_len = uscfp[i]->host.len <  len ? uscfp[i]->host.len : len;
        if (ngx_strncasecmp(uscfp[i]->host.data + (uscfp[i]->host.len - min_len),
                            _this->path + (len - min_len), min_len) != 0)
        {
            ngx_log_error(NGX_LOG_ERR, _this->log, 0,
                          "ngx_http_rpc_start_upsteam check:%s is match:%s with min_len:%d",
                           uscfp[i]->host.data + (uscfp[i]->host.len - min_len ),
                          _this->path + (len - min_len), min_len);
            continue;
        }

        u->conf->upstream = uscfp[i];
    }

    if(u->conf->upstream == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, _this->log, 0,
                      "ngx_http_rpc_start_upsteam task:%p path:%s upstream not find! umcf->upstreams.nelts:%d",
                      _this, _this->path, umcf->upstreams.nelts);

        _this->response_states = NGX_HTTP_INTERNAL_SERVER_ERROR;

        if(_this->exec_in_nginx)
        {
            _this->closure.handler(_this, _this->closure.p1);
        }else{
            ngx_rpc_notify_push_task(_this->proc_notify, &_this->node);
        }

        return;
    }


    u->create_request    = ngx_http_rpc_upstream_create_request;
    u->reinit_request    = ngx_http_rpc_upstream_reinit_request;
    u->process_header    = ngx_http_rpc_upstream_process_status_line;
    u->abort_request     = ngx_http_rpc_upstream_abort_request;
    u->finalize_request  = ngx_http_rpc_upstream_finalize_request;

    u->input_filter_init = ngx_http_rpc_upstream_input_filter_init;
    u->input_filter      = ngx_http_rpc_upstream_non_buffered_copy_filter;
    u->input_filter_ctx  = r;

    ngx_log_error(NGX_LOG_INFO, _this->log, 0,
                  "ngx_http_rpc_start_upsteam task:%p path:%s upstream:%V port:%d",
                  _this, _this->path, &(u->conf->upstream->host), u->conf->upstream->port);

    r->main->count++;

    // start upstream and not forward to client
    r->subrequest_in_memory = 1;
    ngx_http_upstream_init(r);
}

