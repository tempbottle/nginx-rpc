#include "ngx_http_rpc.h"



/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {

    ngx_null_command
};


static void* ngx_http_rpc_create_main_conf(ngx_conf_t *cf);



/* Modules */
static ngx_http_module_t  ngx_http_rpc_module_ctx = {
    NULL,                /* preconfiguration */
    NULL,                /* postconfiguration */
    ngx_http_rpc_create_main_conf,                /* create main configuration */
    NULL,                /* init main configuration */

    NULL,/* create server configuration */
    NULL,/* merge server configuration */

    NULL,                /* create location configuration */
    NULL                 /* merge location configuration */
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



///// Done task
///
#define ngx_http_cycle_get_module_srv_conf(cycle, module)            \
    (cycle->conf_ctx[ngx_http_module.index] ?                        \
    ((ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index]) \
    ->srv_conf[module.ctx_index] : NULL)



static void* ngx_http_rpc_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *)
            ngx_pcalloc(cf->pool,sizeof(ngx_http_rpc_conf_t));

    if(conf == NULL)
    {
        return NULL;
    }

    conf->proc_queue = NULL;
    conf->notify     = NULL;

    conf->log = ngx_cycle->log;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "ngx_http_rpc_create_srv_conf :%p", conf);
    return conf;
}



static void ngx_http_rpc_process_notify_task(void *ctx);

static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle)
{

    ngx_http_rpc_conf_t *rpc_conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_proc_rpc_conf_t *proc_conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(rpc_conf == NULL || proc_conf == NULL)
        return NGX_OK;


    rpc_conf->log = ngx_cycle->log;
    rpc_conf->proc_queue = proc_conf->queue;

    rpc_conf->notify = ngx_rpc_queue_add_current_producer(rpc_conf->proc_queue, rpc_conf);

    rpc_conf->notify->read_hanlder = ngx_http_rpc_process_notify_task;
    rpc_conf->notify->write_hanlder = ngx_http_rpc_process_notify_task;

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0,
                  "ngx_http_rpc_init_process rpc_conf:%p queue:%p notify :%p eventfd:%p",
                  rpc_conf, rpc_conf->proc_queue, rpc_conf->notify, rpc_conf->notify->event_fd);
    return NGX_OK;
}


static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_rpc_notify_unregister(conf->notify);

    ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "ngx_http_rpc_exit_process conf:%p", conf);
}






static void ngx_http_rpc_process_notify_task(void *ctx)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *) ctx;

    ngx_queue_t* pending = NULL;

    while(ngx_rpc_notify_pop_task(conf->notify, &pending) == NGX_OK)
    {
        ngx_rpc_task_t * task = ngx_queue_data(pending, ngx_rpc_task_t, node);

        task->finish.handler(task, task->finish.p1);

        ngx_log_debug(NGX_LOG_DEBUG_HTTP, conf->log, 0 ,
                      "ngx_http_rpc_process_notify_task rpc_conf:%p, notify_eventfd:%d task:%p done_eventfd:%d ",
                      conf, conf->notify->event_fd, task, task->done_notify->event_fd);
    }
}




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

    if(task->res_length > 0)
    {
        //find last
        ngx_chain_t *ptr = &task->res_bufs;

        for( ; ptr->next != NULL; ptr = ptr->next);
        // set last
        ptr->buf->last_buf = 1;

        rc = ngx_http_output_filter(r, &task->res_bufs);
    }

    // clear the task
    ngx_pool_cleanup_t *p1 = ngx_pool_cleanup_add(r->connection->pool, 0);
    p1->data = task;
    p1->handler = ngx_http_rpc_task_destory;

    ngx_log_error(NGX_LOG_INFO, task->log, 0,
                  "ngx_http_inspect_application_finish task:%p status:%d size:%d, rc:%d",
                  task, task->response_states, task->res_length,rc );

    ngx_http_finalize_request(r, rc);
}



void ngx_http_rpc_request_foward(ngx_rpc_task_t* _this, void *ctx)
{

    ngx_rpc_task_t* task = _this;
    ngx_http_request_t *r = ctx;

    // reuse the task





}


void ngx_http_rpc_request_foward_done(ngx_rpc_task_t* _this, void *ctx){

}
