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



static void ngx_http_rpc_process_notify_task(void *ctx)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *) ctx;
    ngx_rpc_task_queue_t* queue = conf->done_queue;

    ngx_queue_t* pending = NULL;

    ngx_shmtx_lock(&queue->next_lock);

    if(!ngx_queue_empty(&queue->next))
    {
        pending = queue->next.next;
        ngx_queue_remove(&queue->next);
        ngx_queue_init(&queue->next);
    }

    ngx_log_error(NGX_LOG_INFO, conf->log, 0,
                  "ngx_http_rpc_process_notify_task conf:%p queue:%p pending:%p ",conf , queue, pending);
    ngx_shmtx_unlock(&queue->next_lock);

    if(pending == NULL)
        return;

    for( pending->prev->next = NULL ;
         pending != NULL;
         pending = pending->next)
    {
        ngx_rpc_task_t *task = ngx_queue_data(pending, ngx_rpc_task_t, done);
        task->finish.handler(task, task->finish.p1);
        ngx_log_error(NGX_LOG_INFO, conf->log, 0,
                      "ngx_http_rpc_process_notify_task conf:%p queue:%p task:%p", conf, queue, pending);
    }

}

static void* ngx_http_rpc_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *)
            ngx_pcalloc(cf->pool,sizeof(ngx_http_rpc_conf_t));

    if(conf == NULL)
    {
        return NULL;
    }

    conf->proc_queue = NULL;
    conf->done_queue = NULL;


    conf->log = ngx_cycle->log;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "ngx_http_rpc_create_srv_conf :%p", conf);
    return conf;
}


static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *rpc_conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_proc_rpc_conf_t *proc_conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(rpc_conf == NULL || proc_conf == NULL)
        return NGX_OK;


    rpc_conf->log = ngx_cycle->log;
    rpc_conf->proc_queue = proc_conf->queue;

    rpc_conf->done_queue = ngx_http_rpc_task_queue_create(rpc_conf->proc_queue->pool);

    rpc_conf->done_queue->notify =  ngx_rpc_queue_add_current_producer(rpc_conf->proc_queue, rpc_conf);

    rpc_conf->done_queue->notify->read_hanlder = ngx_http_rpc_process_notify_task;
    rpc_conf->done_queue->notify->write_hanlder = ngx_http_rpc_process_notify_task;
    rpc_conf->done_queue->notify->ctx = rpc_conf;

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0,
                  "ngx_http_rpc_init_process rpc_conf:%p queue:%p notify:%p done_queue:%p",
                  rpc_conf, rpc_conf->proc_queue, rpc_conf->done_queue->notify, rpc_conf->done_queue);
    return NGX_OK;
}


static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_http_rpc_task_queue_destory(conf->done_queue);

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0, "ngx_http_rpc_exit_process conf:%p",conf);
}











