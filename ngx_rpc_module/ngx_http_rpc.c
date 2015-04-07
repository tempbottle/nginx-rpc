#include "ngx_http_rpc.h"

static char * ngx_http_rpc_conf_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {

    { ngx_string("ngx_http_rpc_shm_size"),
      NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
      ngx_http_rpc_conf_set_shm_size,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};


static void* ngx_http_rpc_create_main_conf(ngx_conf_t *cf);
static char* ngx_http_rpc_init_main_conf(ngx_conf_t *cf, void *conf);


/* Modules */
static ngx_http_module_t  ngx_http_rpc_module_ctx = {
    NULL,                /* preconfiguration */
    NULL,                /* postconfiguration */
    ngx_http_rpc_create_main_conf,                /* create main configuration */
    ngx_http_rpc_init_main_conf,                /* init main configuration */

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

    ngx_rpc_notify_t *notify = conf->notify;

    ngx_queue_t task_nofity;
    // processing pending notify
    ngx_shmtx_lock(&notify->lock_task);
    if(!ngx_queue_empty(&notify->task))
    {
        ngx_queue_split(&(notify->task), notify->task.next, &task_nofity);
    }
    ngx_shmtx_unlock(&notify->lock_task);

    ngx_queue_t *p = NULL;
    for(p = task_nofity.next; p != &task_nofity; )
    {
        ngx_rpc_notify_task_t *t = ngx_queue_data(p, ngx_rpc_notify_task_t, next);

        if(t->hanlder)
            t->hanlder(t->ctx);
        else{
            ngx_rpc_task_t * task = t->ctx;
           task_closure_exec(task);
        }

        ngx_slab_free_locked(notify->shpool, p);
    }
}


static ngx_int_t ngx_http_rpc_init_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0, "ngx_http_rpc_init_process conf:%p",conf);

    if(conf == NULL)
        return NGX_OK;

    conf->notify = ngx_rpc_notify_create(conf->shpool, conf);

    conf->notify->read_hanlder = ngx_http_rpc_process_notify_task;

    ngx_log_error(NGX_LOG_WARN,cycle->log, 0, "ngx_http_rpc_init_process conf:%p",conf);
    return NGX_OK;
}


static void ngx_http_rpc_exit_process(ngx_cycle_t *cycle)
{
    ngx_http_rpc_conf_t *conf =
            ngx_http_cycle_get_module_main_conf(cycle, ngx_http_rpc_module);

    if(conf)
        ngx_rpc_notify_destory(conf->notify);

     ngx_log_error(NGX_LOG_WARN,cycle->log, 0, "ngx_http_rpc_exit_process conf:%p",conf);
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
        return NGX_OK;
    }

    ctx->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;
    ctx->shpool->data = ctx;
    ngx_log_error(NGX_LOG_WARN, shm_zone->shm.log, 0, "ngx_http_rpc_conf_set_shm_size :%d, pool:%p", ctx->shm_size, ctx->shpool);
    return NGX_OK;
}

static char *ngx_http_rpc_conf_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
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

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "ngx_http_rpc_conf_set_shm_size :%d", size);
    return NGX_CONF_OK;
}



static void* ngx_http_rpc_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *)
            ngx_pcalloc(cf->pool,sizeof(ngx_http_rpc_conf_t));

    if(conf == NULL)
    {
        return NULL;
    }

    conf->shm_size = NGX_CONF_UNSET_UINT;
    conf->shpool = NULL;
    conf->notify = NULL;


    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "ngx_http_rpc_create_srv_conf :%p", conf);
    return conf;
}

char* ngx_http_rpc_init_main_conf(ngx_conf_t *cf, void *conf){

    ngx_http_rpc_conf_t *children = (ngx_http_rpc_conf_t *)conf;

    if(children->shm_size != NGX_CONF_UNSET_UINT)
        return NGX_CONF_OK;

    static ngx_str_t ngx_http_rpc = ngx_string("ngx_http_rpc_shm");

    children->shm_size = 4096*1024*256;

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &ngx_http_rpc, children->shm_size,
                                                     &ngx_http_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = children;

    ngx_conf_log_error(NGX_LOG_WARN,cf,0,"ngx_http_rpc_merge_srv_conf size:%d", children->shm_size);

    return NGX_CONF_OK;
}





