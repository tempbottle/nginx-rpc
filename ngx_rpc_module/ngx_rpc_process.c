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
    ngx_log_t *                log;

} ngx_proc_rpc_conf_t;



/// commands
static char * ngx_proc_rpc_set_shm_size(ngx_conf_t *cf,
                                        ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_proc_rpc_commands[] = {

    { ngx_string("ngx_proc_rpc_queue_capacity"),
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

    { ngx_string("ngx_proc_rpc_log"),
      NGX_PROC_CONF|NGX_CONF_TAKE1,
      ngx_proc_rpc_set_log_file,
      NGX_PROC_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};



static void *ngx_proc_rpc_create_loc_conf(ngx_conf_t *cf);
static char *ngx_proc_rpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);


static ngx_proc_module_t ngx_proc_rpc_module_ctx = {
    ngx_string("ngx_proc_rpc"),
    NULL,
    NULL,
    ngx_proc_rpc_create_loc_conf,
    ngx_proc_rpc_merge_loc_conf,
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
    ngx_proc_rpc_master_exit,
    NGX_MODULE_V1_PADDING
};


int ngx_rpc_process_push_task(ngx_rpc_task_t *task)
{
    ngx_proc_rpc_conf_t *conf =
            ngx_proc_get_conf(ngx_cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_http_rpc_task_ref_add(task);

    if(!ngx_rpc_queue_push(conf->queue, task))
    {
        ngx_http_rpc_task_ref_sub(task);
        return NGX_ERROR;
    }

    return NGX_OK;
}


static ngx_int_t ngx_proc_rpc_process_one_task(ngx_rpc_task_t * task)
{
    task_closure_exec(task);

    ngx_http_rpc_task_ref_sub(task);

    return NGX_OK;
}


static void ngx_proc_rpc_process_one_cycle(void * proc)
{

    ngx_proc_rpc_conf_t *process= (ngx_proc_rpc_conf_t *)proc;
    ngx_rpc_notify_t * notify = process->notify;

    ngx_queue_t task_nofity;

    //processing pending nofity
    ngx_shmtx_lock(&notify->lock_task);

    if(!ngx_queue_empty(&notify->task))
    {
        ngx_queue_split(&notify->task, notify->task.next, &task_nofity);
    }

    ngx_shmtx_unlock(&notify->lock_task);

    ngx_queue_t *ptr = NULL;
    for(ptr = task_nofity.next; ptr != &task_nofity; )
    {
        ngx_rpc_notify_task_t *t = ngx_queue_data(ptr, ngx_rpc_notify_task_t, next);

        ngx_proc_rpc_process_one_task((ngx_rpc_task_t*)t->ctx);

        ptr = ptr->next;
        ngx_slab_free_locked(notify->shpool, ptr);
    }

    ngx_rpc_task_t * task = NULL;
    for( task = ngx_rpc_queue_pop(process->queue, process);
         task != NULL;
         task = ngx_rpc_queue_pop(process->queue, process))
    {
        ngx_proc_rpc_process_one_task(task);
    }
}


static void ngx_proc_rpc_wait_task(void * proc, void *task){
    // do nothing
}

static void ngx_proc_rpc_notify_task(void * proc, void *task)
{
    ngx_proc_rpc_conf_t *p = (ngx_proc_rpc_conf_t *) proc;
    ngx_rpc_notify_task(p->notify, NULL, task);
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

    c->shm_size = size;

    static ngx_str_t rpc_shm_name = ngx_string("ngx_proc_rpc");

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &rpc_shm_name, c->shm_size,
                                                     &ngx_proc_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = c;
    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_set_shm_size %d", size);

    return NGX_CONF_OK;

}


static void *ngx_proc_rpc_create_loc_conf(ngx_conf_t *cf)
{

    ngx_proc_rpc_conf_t * conf = ngx_pcalloc(cf->pool, sizeof(ngx_proc_rpc_conf_t));

    if(NULL == conf)
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
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

static char *ngx_proc_rpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_proc_rpc_conf_t *rpc_conf = (ngx_proc_rpc_conf_t *) child;

    if(rpc_conf->queue_capacity == NGX_CONF_UNSET_UINT)
    {
        rpc_conf->queue_capacity = 256;
    }

    if(rpc_conf->shm_size != NGX_CONF_UNSET_UINT)
        return NGX_CONF_OK;

    rpc_conf->shm_size = 4096*1024*256;


    static ngx_str_t rpc_shm_name = ngx_string("ngx_proc_rpc");

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &rpc_shm_name, rpc_conf->shm_size,
                                                     &ngx_proc_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = rpc_conf;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_set_shm_size %d by default", rpc_conf->shm_size);
    return NGX_CONF_OK;
}



static ngx_int_t ngx_proc_rpc_process_init(ngx_cycle_t *cycle)
{

    ngx_proc_rpc_conf_t * conf= ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(conf == NULL)
        return NGX_OK;

    conf->notify = ngx_rpc_notify_create(conf->shpool, conf);

    if(0 == conf->notify)
    {
        return NGX_ERROR;
    }

    conf->notify->read_hanlder = ngx_proc_rpc_process_one_cycle;
    conf->notify->ctx = conf;


    conf->queue = ngx_rpc_queue_create(conf->shpool, conf->queue_capacity);

    conf->queue->notify = ngx_proc_rpc_notify_task;
    conf->queue->wait = ngx_proc_rpc_wait_task;

    if(conf->queue == NULL)
    {
         ngx_log_error(NGX_LOG_INFO,cycle->log, 0, "conf->queue ");

        return NGX_ERROR;
    }


     ngx_log_error(NGX_LOG_INFO,cycle->log, 0, "ngx_http_rpc_init_process done:");

    return 0;
}

static void ngx_proc_rpc_process_exit(ngx_cycle_t *cycle)
{

    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "ngx_proc_rpc_process_exit conf:%p", conf);

    if(conf)
        ngx_rpc_notify_destory(conf->notify);
}


static void  ngx_proc_rpc_master_exit (ngx_cycle_t *cycle){

    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

     ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "ngx_proc_rpc_master_exit conf:%p", conf);
    if(conf)
        ngx_rpc_queue_destory(conf->queue);
}
