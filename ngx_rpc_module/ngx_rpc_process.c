#include "ngx_rpc_queue.h"
#include "ngx_rpc_notify.h"

#include "ngx_rpc_process.h"



// every process has its memery
typedef struct {
    ngx_slab_pool_t           *shpool;
    ngx_rpc_queue_t           *queue;
    ngx_rpc_notify_t          *notify;

    ngx_uint_t                shm_size;
    ngx_uint_t                queue_capacity;

} ngx_proc_rpc_conf_t;



/// commands
static char * ngx_proc_rpc_set_shm_size(ngx_conf_t *cf,
                                        ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_proc_rpc_commands[] = {

    { ngx_string("queue_capacity"),
      NGX_PROC_CONF | NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_PROC_CONF_OFFSET,
      offsetof(ngx_proc_rpc_conf_t, queue_capacity),
      NULL },

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
static void      ngx_proc_rpc_master_exit (ngx_cycle_t *cycle);

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


bool  ngx_rpc_process_push_task(ngx_rpc_task_t *task)
{
    ngx_proc_rpc_conf_t *conf =  ngx_proc_get_conf(ngx_cycle->conf_ctx, ngx_proc_rpc_module);
    return ngx_rpc_queue_push(conf->queue, task);
}


static ngx_int_t ngx_proc_rpc_process_one_task(ngx_rpc_task_t * task)
{
    task->filter(task->ctx,task, task->pre_task);
    ngx_http_rpc_task_ref_sub(task);
}


static ngx_int_t ngx_proc_rpc_process_one_cycle(void * proc)
{

    ngx_proc_rpc_conf_t *p= (ngx_proc_rpc_conf_t *)proc;
    ngx_rpc_notify_t * notify = p->notify;

    ngx_queue_t task_nofity;

    // processing pending nofity
    ngx_shmtx_lock(&notify->lock_task);

    if(!ngx_queue_empty(notify->task))
    {
        ngx_queue_split(&notify->task, &notify->task.next, &task_nofity);
    }

    ngx_shmtx_unlock(&notify->lock_task);

    for( ngx_queue_t *p = task_nofity.next; p != &task_nofity; )
    {
        ngx_rpc_notify_task_t *t = ngx_queue_data(p, ngx_rpc_notify_task_t, node);

        ngx_proc_rpc_process_one_task((ngx_rpc_task_t*)t->ctx);

        p = p->next;
        ngx_slab_free_locked(notify->shpool, p);
    }

    for( ngx_rpc_task_t * task = ngx_rpc_queue_pop(p->queue, p);
         task != NULL;
         task = ngx_rpc_queue_pop(p->queue, p))
    {

        ngx_proc_rpc_process_one_task(task);
    }

    return 0;
}


static ngx_int_t ngx_proc_rpc_wait_task(void * proc, void *task){

    // do nothing
    return 0;
}

static ngx_int_t ngx_proc_rpc_notify_task(void * proc, void *task){

    ngx_proc_rpc_conf_t *p = (ngx_proc_rpc_conf_t *) proc;
    ngx_rpc_notify_task(p->notify, NULL, task);
    return 0;
}




static ngx_int_t ngx_proc_rpc_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_proc_rpc_conf_t       *ctx, *octx;

    octx = data;
    ctx = shm_zone->data;

    //TODO on restart
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

    ctx->queue = ngx_rpc_queue_create(ctx->shpool, ctx->queue_capacity);

    ctx->queue->notify = ngx_proc_rpc_notify_task;
    ctx->queue->wait = ngx_proc_rpc_wait_task;

    if(ctx->queue == NULL)
    {
        ngx_log_error(NGX_LOG_EMERG, shm_zone->shm.log, 0,
                      "ngx_rpc_queue_create failed ");
        return NGX_ERROR;
    }

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
    conf->queue_capacity = NGX_CONF_UNSET_UINT;

    conf->shpool  = NULL;
    conf->notify = NULL;
    conf->queue = NULL;

    return  conf;
}




static ngx_int_t ngx_proc_rpc_process_init(ngx_cycle_t *cycle)
{

    ngx_proc_rpc_conf_t * conf = ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    conf->notify = ngx_rpc_notify_create(conf->shpool, conf);\

    if(0 == conf->notify)
    {
        return NGX_ERROR;
    }

    conf->process->notify.read_hanlder = ngx_proc_rpc_process_one_cycle;
    conf->process->notify.ctx = conf;

    return 0;
}

static void  ngx_proc_rpc_process_exit(ngx_cycle_t *cycle)
{
    // do nothing ?

    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_rpc_notify_destory(&conf->notify);

}


static void      ngx_proc_rpc_master_exit (ngx_cycle_t *cycle){

    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_rpc_notify_destory(&conf->queue);
}
