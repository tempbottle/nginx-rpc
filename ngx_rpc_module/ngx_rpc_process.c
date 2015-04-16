#include "ngx_rpc_queue.h"
#include "ngx_rpc_notify.h"

#include "ngx_rpc_process.h"


/// commands
static char * ngx_proc_rpc_set_shm_size(ngx_conf_t *cf,
                                        ngx_command_t *cmd, void *conf);

static char * ngx_proc_rpc_set_log(ngx_conf_t *cf,
                                   ngx_command_t *cmd, void *conf);



static ngx_command_t ngx_proc_rpc_commands[] = {

    { ngx_string("ngx_proc_rpc_set_shm_size"),
      NGX_PROC_CONF|NGX_CONF_TAKE1,
      ngx_proc_rpc_set_shm_size,
      NGX_PROC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("ngx_proc_rpc_set_log"),
      NGX_PROC_CONF|NGX_CONF_TAKE1,
      ngx_proc_rpc_set_log,
      NGX_PROC_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};



static void *ngx_proc_rpc_create_loc_conf(ngx_conf_t *cf);
static char *ngx_proc_rpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_int_t ngx_proc_rpc_process_init(ngx_cycle_t *cycle);
static void      ngx_proc_rpc_process_exit(ngx_cycle_t *cycle);

static ngx_proc_module_t ngx_proc_rpc_module_ctx = {
    ngx_string("ngx_proc_rpc"),
    NULL,
    NULL,
    ngx_proc_rpc_create_loc_conf,
    ngx_proc_rpc_merge_loc_conf,
    NULL, // prepare
    ngx_proc_rpc_process_init,
    NULL,
    ngx_proc_rpc_process_exit
};


static ngx_int_t  ngx_proc_rpc_master_init(ngx_cycle_t *cycle);
static void  ngx_proc_rpc_master_exit (ngx_cycle_t *cycle);

ngx_module_t ngx_proc_rpc_module = {
    NGX_MODULE_V1,
    &ngx_proc_rpc_module_ctx,
    ngx_proc_rpc_commands,
    NGX_PROC_MODULE,
    NULL, // master NULL
    ngx_proc_rpc_master_init, // init module called in master instead
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_proc_rpc_master_exit,
    NGX_MODULE_V1_PADDING
};



static void ngx_proc_rpc_process_one_cycle(void * conf)
{

    ngx_proc_rpc_conf_t * rpc_conf= (ngx_proc_rpc_conf_t *)conf;
    ngx_rpc_processor_t * p = rpc_conf->processor;

    ngx_rpc_task_t * task = ngx_atomic_swap_set(p->ptr, NULL);

    if(task != NULL )
    {
        task->closure.handler(task->closure.p1);

        ngx_shmtx_lock(task->done_queue->next_lock);
        ngx_queue_add(task->done_queue->next, task->done);
        ngx_shmtx_unlock(task->done_queue->next_lock);
    }

    ngx_log_error(NGX_LOG_INFO, rpc_conf->log, 0 ,
                  "ngx_proc_rpc_process_one_cycle rpc_conf:%p, processor:%p, task:%p",
                  rpc_conf, p, task);

    ngx_shmtx_lock(rpc_conf->queue->procs_lock);
    ngx_queue_add(rpc_conf->queue->idles, p->next);
    ngx_shmtx_unlock(rpc_conf->queue->procs_lock);
}


static ngx_int_t ngx_proc_rpc_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_proc_rpc_conf_t       *ctx, *octx;

    octx = data;
    ctx = shm_zone->data;

    ngx_log_debug(NGX_LOG_DEBUG, shm_zone->shm.log, 0, "ngx_proc_rpc_init_zone  conf:%p shm_pool:%p data:%p",
                  shm_zone->data, shm_zone->shm.addr, data);

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

    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_set_shm_size %d , \
                  conf:%p, queue:%p",
                  size, c, c->queue);

    return NGX_CONF_OK;

}

static char *ngx_proc_rpc_set_log(ngx_conf_t *cf,
                                  ngx_command_t *cmd, void *conf)
{
    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_set_log ");
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

    conf->shpool  = NULL;
    conf->queue   = NULL;
    conf->log     = cf->log;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_create_loc_conf conf:%p",conf);
    return conf;
}

static char *ngx_proc_rpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_proc_rpc_conf_t *rpc_conf = (ngx_proc_rpc_conf_t *) child;

    if(rpc_conf->shm_size != NGX_CONF_UNSET_UINT)
    {

        return NGX_CONF_OK;
    }

    // 1 GB
    rpc_conf->shm_size = 4096*1024*256;

    static ngx_str_t rpc_shm_name = ngx_string("ngx_proc_rpc");

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &rpc_shm_name, rpc_conf->shm_size,
                                                     &ngx_proc_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = rpc_conf;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0 ,
                  "ngx_proc_rpc_set_shm_size %d queue_capacity:%d",
                  rpc_conf->shm_size, rpc_conf->queue_capacity);
    return NGX_CONF_OK;
}




static ngx_int_t  ngx_proc_rpc_master_init(ngx_cycle_t *cycle)
{
    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(conf == NULL)
    {
        ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "Not found ngx_proc_rpc_conf_t in this process");
        return NGX_OK;
    }

    conf->queue = ngx_rpc_queue_create(conf->shpool);
    conf->queue->log = cycle->log;



    ngx_log_error(NGX_LOG_INFO, cycle->log, 0 , "ngx_proc_rpc_init_zone, \
                  conf:%p, queue:%p,queue_capacity:%d ",
                  conf, conf->queue, conf->queue_capacity);
    return NGX_OK;
}

static void  ngx_proc_rpc_master_exit (ngx_cycle_t *cycle){

    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(conf)
    {
        ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "ngx_proc_rpc_master_exit conf:%p queue:%p", conf, conf->queue);
        ngx_rpc_queue_destory(conf->queue);
    }

}

static ngx_int_t ngx_proc_rpc_process_init(ngx_cycle_t *cycle)
{

    ngx_proc_rpc_conf_t * conf= ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(conf == NULL)
        return NGX_OK;

     conf->processor = ngx_slab_alloc(conf->shpool, sizeof(ngx_rpc_processor_t));

    conf->processor->notify = ngx_rpc_queue_add_current_producer(conf->queue);

    conf->processor->notify->ctx = conf;
    conf->processor->notify->read_hanlder = ngx_proc_rpc_process_one_cycle;
    conf->processor->notify->read_hanlder = ngx_proc_rpc_process_one_cycle;

    ngx_log_error(NGX_LOG_INFO,cycle->log, 0,
                  "ngx_http_rpc_init_process, conf:%p queue:%p processor:%p nofiy:%p",
                  conf, conf->queue, conf->processor, conf->processor->notify);

    return 0;
}

static void ngx_proc_rpc_process_exit(ngx_cycle_t *cycle)
{
    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);
    ngx_slab_free(conf->shpool, conf->processor);

    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, " ngx_proc_rpc_process_exit processor:%p", conf->processor);
}
