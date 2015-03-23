#include "ngx_rpc_process.h"


#include "ngx_rpc_queue.h"
#include "ngx_rpc_notify.h"


/// every process has a struct
typedef struct {
    ngx_slab_pool_t *shpool;
    ngx_rpc_queue_t *queue;
    ngx_rpc_notify_t notify;
} ngx_proc_rpc_process_t;



// every process has its memery
typedef struct {
    ngx_uint_t                shm_size;
    ngx_slab_pool_t           *shpool;
    ngx_proc_rpc_process_t    *process;
} ngx_proc_rpc_conf_t;



/// commands
static char * ngx_proc_rpc_set_shm_size(ngx_conf_t *cf,
                                        ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_proc_rpc_commands[] = {

    { ngx_string("ngx_proc_rpc_shm_size"),
      NGX_PROC_CONF|NGX_CONF_TAKE1,
      ngx_proc_rpc_set_shm_size,
      NGX_PROC_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};



static void *ngx_proc_rpc_create_conf(ngx_conf_t *cf);

static ngx_proc_module_t ngx_proc_rpc_module_ctx = {
    ngx_string("ngx_proc_rpc"),
    NULL,
    NULL,
    ngx_proc_rpc_create_conf,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static ngx_int_t ngx_proc_rpc_process_init(ngx_cycle_t *cycle);
static void      ngx_proc_rpc_process_exit(ngx_cycle_t *cycle);

ngx_module_t ngx_proc_rpc_module = {
    NGX_MODULE_V1,
    &ngx_proc_rpc_module_ctx,
    ngx_proc_rpc_commands,
    NGX_PROC_MODULE,
    NULL,
    NULL,
    ngx_proc_rpc_process_init,
    NULL,
    NULL,
    ngx_proc_rpc_process_exit,
    NULL,
    NGX_MODULE_V1_PADDING
};



static ngx_int_t ngx_proc_rpc_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_proc_rpc_conf_t       *ctx, *octx;

    octx = data;
    ctx = shm_zone->data;

    if (octx != NULL) {
        if (ctx->shm_size !=  octx->shm_size) {
            ngx_log_error(NGX_LOG_EMERG, shm_zone->shm.log, 0,
                          "reqstat \"%V\" uses the value str \"%d\" "
                          "while previously it used \"%d\"",
                          &shm_zone->shm.name, ctx->shm_size, octx->shm_size);
            return NGX_ERROR;
        }

        ctx->shpool = octx->shpool;
        ctx->process = octx->process;
        return NGX_OK;
    }

    ctx->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;
    ctx->shpool->data = ctx;

    return NGX_OK;
}

static char * ngx_proc_rpc_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{

    ngx_proc_rpc_conf_t *c = ( ngx_proc_rpc_conf_t *) conf;

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

    static ngx_str_t rpc_shm_name = ngx_string("ngx_proc_rpc");

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &rpc_shm_name, size,
                &ngx_proc_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = c;

   return NGX_CONF_OK;

}


static void *ngx_proc_rpc_create_conf(ngx_conf_t *cf)
{

    ngx_proc_rpc_conf_t * conf = ngx_pcalloc(cf->pool, sizeof(ngx_proc_rpc_conf_t));

    if(NULL == conf)
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf->log, 0,
                           "ngx_proc_rpc_create_conf error");
        return NULL;
    }

    conf->shm_size = NGX_CONF_UNSET_UINT;
    conf->process = NULL;
    conf->shpool  = NULL;

    return  conf;
}




static ngx_int_t ngx_proc_rpc_process_task(void * proc)
{

    ngx_proc_rpc_process_t *p= (ngx_proc_rpc_process_t *)proc;

    void * task =NULL;

    // processing pending nofity

    // process queue
    ngx_rpc_pop_task_block(p->queue, &task, proc);

    if(task)
    {
        ngx_rpc_task_t * t = (ngx_rpc_task_t*)task;
        t->process_hander(t);
        t->next_hander(t);
    }

    return 0;
}


static ngx_int_t ngx_proc_rpc_wait_task(void * proc, void *task){

    // yei;d to epoll
    return 0;
}

static ngx_int_t ngx_proc_rpc_notify_task(void * proc, void *task){

    ngx_proc_rpc_process_t *p = (ngx_proc_rpc_process_t *) proc;
    ngx_rpc_notify_task(p->notify, ngx_proc_rpc_process_task, task);
    return 0;
}


static ngx_int_t ngx_proc_rpc_process_init(ngx_cycle_t *cycle)
{

    ngx_proc_rpc_conf_t * conf = ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    conf->process = ngx_slab_alloc_locked(conf->shpool, sizeof(ngx_proc_rpc_process_t));
    conf->process->shpool = conf->shpool;
    conf->process->queue = ngx_http_rpc_task_queue;

    // this is every procss
    if(0 != ngx_rpc_notify_create(&conf->process->notify, cycle, conf->shpool))
    {
        return NGX_ERROR;
    }

    conf->process->notify.read_hanlder = ngx_proc_rpc_process_task;
    conf->process->notify.ctx = conf->process;

    return 0;
}

static void  ngx_proc_rpc_process_exit(ngx_cycle_t *cycle)
{
    // do nothing ?

    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_rpc_notify_destory(&conf->process->notify);

}
