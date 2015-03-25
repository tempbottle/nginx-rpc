#include "ngx_http_rpc.h"


int ngx_http_header_modify_content_length(ngx_http_request_t *r, ngx_int_t value)
{
    r->headers_in.content_length_n = value;


    ngx_list_part_t *part =  &r->headers_in.headers.part;
    ngx_table_elt_t *header = part->elts;
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
            header = part->elts;
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

            header[i].value.data = ngx_palloc(r->pool, 32);
            header[i].value.len = 32;

            snprintf((char*)header[i].value.data,
                     32, "%ld", value);
            r->headers_in.content_length->value =  header[i].value;

            return NGX_OK;
        }
    }
    return NGX_ERROR;
}

static char * ngx_http_rpc_conf_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {
    { ngx_string("request_capacity"),
      NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_rpc_conf_t, request_capacity),
      NULL },

    { ngx_string("ngx_http_rpc_shm_size"),
      NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
      ngx_http_rpc_conf_set_shm_size,
      NGX_HTTP_SRV_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};


static void* ngx_http_rpc_create_loc_conf(ngx_conf_t *cf);
/* Modules */
static ngx_http_module_t  ngx_http_rpc_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL,                /* create main configuration */
    NULL,                /* init main configuration */

    ngx_http_rpc_create_loc_conf,/* create server configuration */
    NULL,                /* merge server configuration */

    NULL,                /* create location configuration */
    NULL                 /* merge location configuration */
};


static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle);
static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle);
static void ngx_http_rpc_exit_master(ngx_cycle_t *cycle);

ngx_module_t  ngx_http_rpc_module = {
    NGX_MODULE_V1,
    &ngx_http_rpc_module_ctx,              /* module context */
    ngx_http_rpc_module_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but some where called init module instead
    NULL,              /* init module */
    ngx_http_rpc_init_process,             /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_http_rpc_exit_process,             /* exit process */
    ngx_http_rpc_exit_master,             /* exit master */
    NGX_MODULE_V1_PADDING
};



///// Done task
///
#define ngx_http_cycle_get_module_loc_conf(cycle, module)            \
    (cycle->conf_ctx[ngx_http_module.index] ?                        \
    ((ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index]) \
    ->loc_conf[module.ctx_index] : NULL)


static void ngx_http_rpc_process_notify_task(void *ctx)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *) ctx;

    ngx_rpc_notify_t *notify = conf->notify;

    ngx_queue_t task_nofity;
    // processing pending notify
    ngx_shmtx_lock(&notify->lock_task);

    if(!ngx_queue_empty(notify->task))
    {
        ngx_queue_split(&notify->task, &notify->task.next, &task_nofity);
    }

    ngx_shmtx_unlock(&notify->lock_task);

    for( ngx_queue_t *p = task_nofity.next; p != &task_nofity; )
    {
        ngx_rpc_notify_task_t *t = ngx_queue_data(p, ngx_rpc_notify_task_t, node);

        ngx_http_rpc_proccess_task((ngx_rpc_task_t*)t->ctx);
        p = p->next;
        ngx_slab_free_locked(notify->shpool, p);
    }
}


static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle)
{

    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_loc_conf(cycle, ngx_http_rpc_module);

    conf->notify = ngx_rpc_notify_create(conf->shpool, conf);

    conf->notify->read_hanlder = ngx_http_rpc_process_notify_task;

}


static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_loc_conf(cycle, ngx_http_rpc_module);

    ngx_rpc_notify_destory(conf->notify);
}


static void ngx_http_rpc_exit_master(ngx_cycle_t *cycle)
{

}



static ngx_int_t ngx_proc_rpc_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_http_rpc_conf_t       *ctx, *octx;

    octx = data;
    ctx  = shm_zone->data;

    if (octx != NULL) {
        if (ctx->shm_size !=  octx->shm_size) {
            ngx_log_error(NGX_LOG_EMERG, shm_zone->shm.log, 0,
                          "reqstat \"%V\" uses the value str \"%d\" "
                          "while previously it used \"%d\"",
                          &shm_zone->shm.name, ctx->shm_size, octx->shm_size);
            return NGX_ERROR;
        }

        ctx->shpool = octx->shpool;
        ctx->request_capacity = octx->request_capacity;
        return NGX_OK;
    }

    ctx->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;
    ctx->shpool->data = ctx;
    return NGX_OK;
}

static char * ngx_http_rpc_conf_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rpc_conf_t *c = (ngx_http_rpc_conf_t *)conf;

    ngx_str_t  *value = cf->args->elts;
    ssize_t size = ngx_parse_size(&value[1]);

    if (size == NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid zone size \"%V\"", &value[3]);
        return NGX_CONF_ERROR;
    }

    if (size < (ssize_t) (8 * ngx_pagesize)) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "zone \"%V\" is too small", &value[1]);
        return NGX_CONF_ERROR;
    }

    static ngx_str_t ngx_http_rpc = ngx_string("ngx_http_rpc_shm");

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &ngx_http_rpc, size,
                                                     &ngx_http_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = c;

    return NGX_CONF_OK;
}



static void* ngx_http_rpc_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *)
            ngx_pcalloc(cf->pool,sizeof(ngx_http_rpc_conf_t));

    if(conf == NULL)
    {
        ERROR("ngx_http_inspect_conf_t create failed");
        return NULL;
    }

    conf->request_capacity = NGX_CONF_UNSET_UINT;
    conf->shm_size = NGX_CONF_UNSET_UINT;
    conf->shpool = NULL;
    conf->notify = NULL;

    return conf;
}


/////////////////////
static void ngx_http_rpc_proccess_task(ngx_rpc_task_t* task)
{
    ngx_http_rpc_ctx_t *ctx = (ngx_http_rpc_ctx_t *)task->ctx;

    task->filter(ctx->r, task);

    if(task->type == PROCESS_IN_PROC )
    {
        ngx_shmtx_lock(&ctx->task_lock);

        for(ngx_queue_t *ptr = ctx->pending.prev;
              ptr != &ctx->pending;
              ptr = ptr->prev)
        {
                 if(ngx_queue_data(ptr, ngx_rpc_task_t, pending) ==  task)
                 {
                     ngx_queue_remove(ptr);
                 }
        }

        ngx_queue_insert_after(&ctx->done, &task->done);

        ngx_shmtx_lock(&ctx->task_lock);

        ngx_rpc_notify_task(&ctx->notify, NULL, task);

        return;
    }

    ngx_shmtx_lock(&ctx->task_lock);
    ngx_queue_insert_after(&ctx->done, &task->done);
    ngx_shmtx_lock(&ctx->task_lock);
}


static void ngx_http_rpc_dispatcher_task(ngx_rpc_task_t* task)
{

    // 2 do or foward
    switch (task->type) {

    case PROCESS_IN_PROC:
        ngx_shmtx_lock(&ctx->task_lock);
        ngx_queue_insert_after(&ctx->pending, &task->pending);
        ngx_http_rpc_task_ref_add(task->refcount);
        ngx_shmtx_lock(&ctx->task_lock);
        ngx_rpc_process_push_task(task);
        break;

    default:
        ngx_http_rpc_proccess_task(task);
        break;
    }
}



ngx_rpc_task_t* ngx_http_rpc_post_request_task_init(ngx_http_request_t *r,void * ctx)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_rpc_module);

    // 1 new process task
    ngx_rpc_task_t* task = ngx_http_rpc_task_create(rpc_ctx->shpool, rpc_ctx);

    task->ctx = ctx;
    task->status = TASK_INIT;

    // 2 copy the request bufs

    ngx_chain_t* req_chain = &task->req_bufs;

    for(ngx_chain_t* c= r->request_body->bufs; c; c=c->next )
    {
        int buf_size = c->buf->last - c->buf->pos;
        req_chain->buf = (ngx_buf_t*)ngx_slab_alloc_locked(rpc_ctx->shpool,
                                                           sizeof(ngx_buf_t));

        memcpy(c->buf, req_chain->buf,sizeof(ngx_buf_t));

        req_chain->buf->pos = req_chain->buf->start =
                (u_char*) ngx_slab_alloc_locked(ctx->shpool,buf_size);

        memcpy(c->buf->pos, req_chain->buf->pos,buf_size);
        req_chain->next = (ngx_chain_t*)ngx_slab_alloc_locked(ctx->shpool,
                                                              sizeof(ngx_chain_t));
        req_chain = req_chain->next;
        req_chain->next = NULL;
    }

    return task;

}


ngx_rpc_task_t* ngx_http_rpc_sub_request_task_init(ngx_http_request_t *r, void * ctx)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_rpc_module);

    // 1 new process task
    ngx_rpc_task_t* task = ngx_http_rpc_task_create(rpc_ctx->shpool, rpc_ctx);

    task->ctx = ctx;
    task->status = TASK_INIT;


    // 2 copy the request bufs
    return task;

}



////////// ctx
ngx_http_rpc_ctx_t* ngx_http_rpc_ctx_init(ngx_http_request_t *r, void *ctx)
{

    ngx_http_rpc_conf_t *rpc_conf = (ngx_http_rpc_conf_t *)
            ngx_http_conf_get_module_loc_conf(r, ngx_http_rpc_module);

    ngx_http_rpc_ctx_t *rpc_ctx  = (ngx_http_rpc_ctx_t *)
            ngx_slab_alloc_locked(rpc_conf->shpool, sizeof(ngx_http_rpc_ctx_t));

    if(rpc_ctx == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "ngx_palloc error size:%d",
                      sizeof(ngx_http_rpc_ctx_t));
        return NULL;
    }


    ngx_pool_cleanup_t *p1 = ngx_pool_cleanup_add(r->connection->pool, 0);
    p1->data = rpc_ctx;
    p1->handler = ngx_http_rpc_ctx_free;

    rpc_ctx->shpool = rpc_conf->shpool;
    rpc_ctx->notify = rpc_conf->notify;

    rpc_ctx->r     = r;
    rpc_ctx->r_ctx = ctx;

    ngx_queue_init(&rpc_ctx->pending);
    ngx_queue_init(&rpc_ctx->done);


    if(ngx_shmtx_create(&rpc_ctx->task_lock, &rpc_ctx->sh_lock, NULL) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "ngx_palloc error size:%d",
                      sizeof(ngx_http_inspect_ctx_t));
        return NULL;
    }

    ngx_http_set_ctx(r, rpc_ctx, ngx_http_rpc_module);
}

void ngx_http_rpc_ctx_finish_by_task(void *ctx)
{
    ngx_rpc_task_t* task = (ngx_rpc_task_t*) ctx;

    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t *)task->ctx;

    ngx_http_request_t *r = rpc_ctx->r;

    static ngx_str_t type = ngx_string(" application/x-protobuf");

    r->headers_out.content_type = type;
    r->headers_out.status = task->response_states;
    r->headers_out.content_length_n = task->res_length;

    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;

    ngx_int_t rc = ngx_http_send_header(r);
    rc = ngx_http_output_filter(r, &chain);
    ngx_http_finalize_request(r, rc);
}

void ngx_http_rpc_ctx_free(void* ctx)
{
    ngx_http_rpc_ctx_t *c = (ngx_http_rpc_ctx_t*)ctx;

    // free task

    ngx_slab_free_locked(c->shpool, c);
}



static void ngx_http_inspect_application_requeststatus_handler(void* r, ngx_rpc_task_t *task)
{

}

typedef struct {
    RpcChannel *channel;
    const ::google::protobuf::Message* req;
    ::google::protobuf::Message* res;
    RpcCallHandler handler;
    void (*pre_write_event_handler)(ngx_http_request_t *r);
} sub_request_ctx_t;


static void ngx_http_inspect_application_subrequest_parent(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t *task)
{
    ngx_http_rpc_ctx_t* rpc_ctx =
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);

    sub_request_ctx_t * sub_req_ctx  = (ngx_http_rpc_ctx_t*) rpc_ctx;

     r->write_event_handler = sub_req_ctx->pre_write_event_handler;

     // dispath
    sub_req_ctx->handler(sub_req_ctx->channel, sub_req_ctx->req,
                         sub_req_ctx->res, r->headers_out.status);
}



static void ngx_http_inspect_application_subrequest_done(ngx_http_inspect_ctx_t *ctx, ngx_rpc_task_t *task)
{
    ngx_http_request_t* child_req = r;
    ngx_http_request_t* parent_req = r->parent;
    parent_req->headers_out.status  = child_req->headers_out.status;

    sub_request_ctx_t * sub_req_ctx = (sub_request_ctx_t *)data;
    ngx_http_rpc_ctx_t* rpc_ctx =
            ngx_http_get_module_ctx(r, ngx_http_rpc_module);
    rpc_ctx ->sub_req_ctx = sub_req_ctx;

    ngx_http_upstream_t* rup = r->upstream;

    if(child_req->headers_out.status != NGX_HTTP_OK)
    {
         ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                     "subrequest failed,r->headers_out.status:%d rc:%d", r->headers_out.status, rc);
         pr->headers_out.status =  child_req->headers_out.status;

         sub_req_ctx->pre_write_event_handler = parent_req->write_event_handler;
         parent_req->write_event_handler = parent_handler;

         return;
    }

    NgxChainBufferReader reader(*rup->out_bufs, parent_req->pool);

    if(!sub_req_ctx->res->ParseFromZeroCopyStream(&reader))
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "subrequest failed,r->headers_out.status:%d rc:%d", r->headers_out.status, rc);
        pr->headers_out.status = NGX_HTTP_BAD_GATEWAY;
    }



    sub_req_ctx->pre_write_event_handler = parent_req->write_event_handler;
    parent_req->write_event_handler = parent_handler;
}


void ngx_http_inspect_application_subrequest_begin(void *ctx, ngx_rpc_task_t *task)
{
    ngx_http_rpc_ctx_t *rpc_ctx = (ngx_http_rpc_ctx_t*)ctx;
    ngx_http_request_t* r = (ngx_http_request_t*)rpc_ctx->r;

    ngx_http_post_subrequest_t *psr =
            ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));

    if(psr == NULL)
    {
        //TODO finish the request
        return;
    }

    r->request_body->bufs = task->req_bufs;
    ngx_http_header_modify_content_length(r, task->res_length);

    ngx_str_t forward = ngx_string(path.c_str());

    ngx_http_request_t *sr;
    ngx_int_t rc = ngx_http_subrequest(r, &forward, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);

    if(rc != NGX_OK)
    {
        ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "ngx_http_subrequest failed:%d", rc);
        delete  ctx;
        handler(this, req, res, NGX_HTTP_INTERNAL_SERVER_ERROR);
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }
}

