#include "ngx_rpc_queue.h"
#include "ngx_rpc_notify.h"

#include "ngx_rpc_process.h"


/// commands
static char * ngx_proc_rpc_set_shm_size(ngx_conf_t *cf,
                                        ngx_command_t *cmd, void *conf);

static char * ngx_proc_rpc_set_log(ngx_conf_t *cf,
                                   ngx_command_t *cmd, void *conf);



static ngx_command_t ngx_proc_rpc_commands[] = {

    { ngx_string("ngx_proc_rpc_set_queue_capacity"),
      NGX_PROC_CONF | NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_PROC_CONF_OFFSET,
      offsetof(ngx_proc_rpc_conf_t, queue_capacity),
      NULL },

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



static void ngx_proc_rpc_process_one_cycle(void * proc)
{

    ngx_proc_rpc_conf_t *process= (ngx_proc_rpc_conf_t *)proc;
    ngx_rpc_notify_t * notify = process->queue->notify_slot[process->proc_id];

    ngx_log_error(NGX_LOG_DEBUG, notify->log, 0,
                  "ngx_proc_rpc_process_one_cycle proc:%p queue:%p id:%d ",
                  process , process->queue,process->proc_id);

    ngx_queue_t* task_nofity = NULL;

    //processing pending nofity
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

        ngx_log_error(NGX_LOG_DEBUG, notify->log, 0, "process task_nofity ngx_rpc_notify_task_t:%p task:%p, refcount:%d", t, task, task->refcount);

        ngx_slab_free(notify->shpool, ptr->prev);
    }


    ngx_rpc_task_t * task = NULL;
    for( task = ngx_rpc_queue_pop(process->queue, process);
         task != NULL;
         task = ngx_rpc_queue_pop(process->queue, process))
    {
        ngx_log_error(NGX_LOG_DEBUG, notify->log, 0, "process->queue:%p task:%p, refcount:%d", process->queue, task, task->refcount);

        task_closure_exec(task);
        ngx_http_rpc_task_ref_sub(task);
    }
}


static void ngx_proc_rpc_wait_task(void * proc, void *task){

    ngx_proc_rpc_conf_t *process= (ngx_proc_rpc_conf_t *)proc;
    // do nothing
    ngx_log_error(NGX_LOG_DEBUG, process->log, 0, "ngx_proc_rpc_wait_task ngx_proc_rpc_conf_t:%p id:%d", process, process->proc_id);
}

static void ngx_proc_rpc_notify_task(void * proc, void *task)
{
    ngx_proc_rpc_conf_t *p = (ngx_proc_rpc_conf_t *) proc;
    ngx_rpc_task_t* t = (ngx_rpc_task_t*)task;
    ngx_rpc_notify_t * notify = p->queue->notify_slot[p->proc_id];

    ngx_log_error(NGX_LOG_DEBUG, p->log, 0, "ngx_proc_rpc_notify_task ngx_proc_rpc_conf_t:%p id:%d task:%p, refcount:%d", p, p->proc_id, t, t->refcount );

    ngx_rpc_notify_task(notify, NULL, t);
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

    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_set_shm_size %d , \
                  conf:%p, queue:%p,queue_capacity:%d ",
                  size, c, c->queue, c->queue_capacity);

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
    conf->queue_capacity = NGX_CONF_UNSET_UINT;

    conf->shpool  = NULL;
    conf->queue   = NULL;
    conf->log     = cf->log;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_create_loc_conf conf:%p",conf);
    return conf;
}

static char *ngx_proc_rpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_proc_rpc_conf_t *rpc_conf = (ngx_proc_rpc_conf_t *) child;

    if(rpc_conf->queue_capacity == NGX_CONF_UNSET_UINT)
    {
        rpc_conf->queue_capacity = 256;
    }

    if(rpc_conf->shm_size != NGX_CONF_UNSET_UINT)
    {

        return NGX_CONF_OK;
    }

    rpc_conf->shm_size = 4096*1024*256;


    static ngx_str_t rpc_shm_name = ngx_string("ngx_proc_rpc");

    ngx_shm_zone_t *shm_zone = ngx_shared_memory_add(cf, &rpc_shm_name, rpc_conf->shm_size,
                                                     &ngx_proc_rpc_module);

    shm_zone->init = ngx_proc_rpc_init_zone;
    shm_zone->data = rpc_conf;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0 , "ngx_proc_rpc_set_shm_size %d queue_capacity:%d", rpc_conf->shm_size, rpc_conf->queue_capacity);
    return NGX_CONF_OK;
}



static ngx_int_t ngx_proc_rpc_process_init(ngx_cycle_t *cycle)
{

    ngx_proc_rpc_conf_t * conf= ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(conf == NULL)
        return NGX_OK;

    conf->proc_id = ngx_rpc_queue_init_per_process(conf->queue,
                                                   ngx_proc_rpc_process_one_cycle,
                                                   ngx_proc_rpc_process_one_cycle,
                                                   conf);


    ngx_log_error(NGX_LOG_INFO,cycle->log, 0,
                  "ngx_http_rpc_init_process done, queue_capacity:%d shm_size:%d conf:%p proc_id:%p queue:%p",
                  conf->queue_capacity, conf->shm_size, conf, conf->proc_id, conf->queue);

    return 0;
}

static void ngx_proc_rpc_process_exit(ngx_cycle_t *cycle)
{
    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);
    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, " ngx_proc_rpc_process_exit proc_id:%d", conf->proc_id);
}


static ngx_int_t  ngx_proc_rpc_master_init(ngx_cycle_t *cycle)
{
    ngx_proc_rpc_conf_t * conf =  ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    if(conf == NULL)
    {
        ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "Not used ngx_proc_rpc");
        return NGX_OK;
    }

    conf->queue = ngx_rpc_queue_create(conf->shpool, conf->queue_capacity);
    conf->queue->log = cycle->log;
    conf->queue->notify = ngx_proc_rpc_notify_task;
    conf->queue->wait = ngx_proc_rpc_wait_task;

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
