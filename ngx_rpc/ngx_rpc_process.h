#ifndef _ngx_proc_rpcESS_H_
#define _ngx_proc_rpcESS_H_

#include "ngx_rpc_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

///
/// \brief ngx_proc_rpc_create_conf
/// \param cf
/// \return



typedef struct {
    ngx_shmtx_t  *sem;
    int idx;
    ngx_rpc_call_t *shm_call; // if call is null

    ngx_queue_t next_idle;

    int(*run_task)(ngx_rpc_call_t* shm_call);
    int(*init_task)(ngx_rpc_call_t* shm_call);
    int(*destory_task)(ngx_rpc_call_t* shm_call);

} ngx_proc_rpc_process_t;


typedef struct {
    ngx_proc_rpc_process_t* procs;
    ngx_atomic_t procs_count;

    ngx_queue_t idle;
    ngx_shmtx_t lock;

    int(*pop_idle)(ngx_proc_rpc_process_t** proc);
    int(*push_idle)(ngx_proc_rpc_process_t* proc);
    int(*destory_task)(ngx_proc_rpc_process_t* shm_call);
} ngx_proc_pool_t;


static void *ngx_proc_rpc_create_conf(ngx_conf_t *cf);

typedef struct {
    ngx_str_t rpc_proc_conf_file_path;
} ngx_proc_rpc_conf_t;


static ngx_command_t ngx_proc_rpc_commands[] = {

    { ngx_string("rpc_proc_conf_file_path"),
      NGX_PROC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_PROC_CONF_OFFSET,
      offsetof(ngx_proc_rpc_conf_t, rpc_proc_conf_file_path),
      NULL },

      ngx_null_command
};



static ngx_proc_module_t ngx_proc_rpc_module_ctx = {
    ngx_string("ngx_proc_rpc"),
    NULL,
    NULL,
    ngx_proc_rpc_create_conf,
    ngx_proc_rpc_merge_conf,
    ngx_proc_rpc_prepare,
    ngx_proc_rpc_process_init,
    ngx_proc_rpc_loop,
    ngx_proc_rpc_exit_process
};


ngx_module_t ngx_proc_rpc_module = {
    NGX_MODULE_V1,
    &ngx_proc_rpc_module_ctx,
    ngx_proc_rpc_commands,
    NGX_PROC_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};


static void *
ngx_proc_rpc_create_conf(ngx_conf_t *cf)
{
    ngx_proc_rpc_conf_t  *pbcf;

    pbcf = ngx_pcalloc(cf->pool, sizeof(ngx_proc_rpc_conf_t));

    if (pbcf == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "daytime create proc conf error");
        return NULL;
    }

    pbcf->rpc_proc_conf_file_path.data = NULL;
    pbcf->rpc_proc_conf_file_path.len = 0;

    return pbcf;
}


static char *
ngx_proc_rpc_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_proc_rpc_conf_t  *prev = parent;
    ngx_proc_rpc_conf_t  *conf = child;

    ngx_str_t  rpc_conf=ngx_string("ngx_rpc.conf");
    ngx_conf_merge_str_value(conf, prev, rpc_conf);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_proc_rpc_prepare(ngx_cycle_t *cycle)
{
    ngx_proc_rpc_conf_t  *pbcf;

    pbcf = ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log ,"ngx_proc_rpc_prepare:%x",pbcf);

    return NGX_OK;
}


static ngx_int_t
ngx_proc_rpc_process_init(ngx_cycle_t *cycle)
{

    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log ,"ngx_proc_rpc_process_init cycle:%x",cycle);

    // add to event


    return NGX_OK;
}


static ngx_int_t
ngx_proc_rpc_loop(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "daytime %V",
                  &ngx_cached_http_time);

    // lock

    // do

    // unlock

    return NGX_OK;
}


static void
ngx_proc_rpc_exit_process(ngx_cycle_t *cycle)
{
    ngx_proc_rpc_conf_t *pbcf;

    pbcf = ngx_proc_get_conf(cycle->conf_ctx, ngx_proc_rpc_module);

    ngx_close_socket(pbcf->fd);
}


static void
ngx_proc_rpc_accept(ngx_event_t *ev)
{
    u_char             sa[NGX_SOCKADDRLEN], buf[256], *p;
    socklen_t          socklen;
    ngx_socket_t       s;
    ngx_connection_t  *lc;

    socklen = NGX_SOCKADDRLEN;
    lc = ev->data;

    s = accept(lc->fd, (struct sockaddr *) sa, &socklen);
    if (s == -1) {
        return;
    }

    if (ngx_nonblocking(s) == -1) {
        goto finish;
    }

    /*
      Daytime Protocol

      http://tools.ietf.org/html/rfc867

      Weekday, Month Day, Year Time-Zone
    */
    p = ngx_sprintf(buf, "%s, %s, %d, %d, %d:%d:%d-%s",
                    week[ngx_cached_tm->tm_wday],
                    months[ngx_cached_tm->tm_mon],
                    ngx_cached_tm->tm_mday, ngx_cached_tm->tm_year,
                    ngx_cached_tm->tm_hour, ngx_cached_tm->tm_min,
                    ngx_cached_tm->tm_sec, ngx_cached_tm->tm_zone);

    ngx_write_fd(s, buf, p - buf);

finish:
    ngx_close_socket(s);
}

#endif
