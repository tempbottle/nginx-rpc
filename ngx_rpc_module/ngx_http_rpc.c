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

    ngx_rpc_notify_t *notify = conf->queue->notify_slot[conf->proc_id];

    ngx_log_error(NGX_LOG_DEBUG, notify->log, 0,
                  "ngx_proc_rpc_process_one_cycle proc:%p queue:%p notify:%p proc_id:%d ",
                  conf , conf->queue, notify, conf->proc_id);

    ngx_queue_t* task_nofity = NULL;
    ngx_shmtx_lock(&notify->lock_task);
    if(!ngx_queue_empty(&notify->task))
    {
        // swap notify->task and task_nofity
        task_nofity = notify->task.next;
        task_nofity->prev = notify->task.prev;
        task_nofity->prev->next = task_nofity;

        ngx_queue_init(&notify->task);
    }

    ngx_shmtx_unlock(&notify->lock_task);

    ngx_queue_t *ptr = task_nofity;
    for(; ptr != task_nofity; ptr = ptr->next)
    {

        ngx_rpc_notify_task_t *t = ngx_queue_data(ptr, ngx_rpc_notify_task_t, next);

        ngx_rpc_task_t * task = (ngx_rpc_task_t *)t->ctx;
        task_closure_exec(task);
        ngx_http_rpc_task_ref_sub(task);

        ngx_log_error(NGX_LOG_DEBUG, notify->log, 0,
                      "process task_nofity ngx_rpc_notify_task_t:%p task:%p, refcount:%d",
                      t, task, task->refcount);

        ngx_slab_free(notify->shpool, ptr->prev);
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

    conf->queue = NULL;
    conf->proc_id = 0;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "ngx_http_rpc_create_srv_conf :%p", conf);
    return conf;
}


static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *rpc_conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_proc_rpc_conf_t * proc_conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0, "ngx_http_rpc_init_process conf:%p", rpc_conf);

    if(rpc_conf == NULL || proc_conf == NULL)
        return NGX_OK;

    rpc_conf->queue  = proc_conf->queue;

    rpc_conf->proc_id = ngx_rpc_queue_init_per_process(rpc_conf->queue,
                                                       ngx_http_rpc_process_notify_task,
                                                       ngx_http_rpc_process_notify_task, rpc_conf);

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0, "ngx_http_rpc_init_process rpc_conf:%p queue:%p id:%d",
                  rpc_conf, rpc_conf->queue, rpc_conf->proc_id);
    return NGX_OK;
}


static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0, "ngx_http_rpc_exit_process conf:%p",conf);
}











