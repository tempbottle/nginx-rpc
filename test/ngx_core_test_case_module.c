#include <ngx_event.h>
#include <ngx_core.h>
#include <ngx_config.h>


static void *ngx_core_test_case_create_conf(ngx_conf_t *cf);
static char *ngx_core_test_case_merge_conf(ngx_conf_t *cf, void *parent,
                                           void *child);
static ngx_int_t ngx_core_test_case_prepare(ngx_cycle_t *cycle);
static ngx_int_t ngx_core_test_case_process_init(ngx_cycle_t *cycle);
static ngx_int_t ngx_core_test_case_loop(ngx_cycle_t *cycle);
static void ngx_core_test_case_exit_process(ngx_cycle_t *cycle);

extern  int test_main(int argc, char *const *argv, volatile ngx_cycle_t *cycle);




typedef struct {
    ngx_flag_t       enable;
    ngx_uint_t       port;
    ngx_socket_t     fd;
} ngx_core_test_case_conf_t;


static ngx_command_t ngx_core_test_case_commands[] = {


    { ngx_string("test"),
      NGX_PROC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_PROC_CONF_OFFSET,
      offsetof(ngx_core_test_case_conf_t, enable),
      NULL },


    ngx_null_command
};


static ngx_proc_module_t ngx_core_test_case_module_ctx = {
    ngx_string("test_case"),
    NULL,
    NULL,
    ngx_core_test_case_create_conf,
    ngx_core_test_case_merge_conf,
    ngx_core_test_case_prepare,
    ngx_core_test_case_process_init,
    ngx_core_test_case_loop,
    ngx_core_test_case_exit_process
};


ngx_module_t ngx_core_test_case_module = {
    NGX_MODULE_V1,
    &ngx_core_test_case_module_ctx,
    ngx_core_test_case_commands,
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
ngx_core_test_case_create_conf(ngx_conf_t *cf)
{
    ngx_core_test_case_conf_t  *pbcf;

    pbcf = ngx_pcalloc(cf->pool, sizeof(ngx_core_test_case_conf_t));




    cf->log->log_level = 0xFF;


    test_main(0, NULL, cf->cycle );

    if (pbcf == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "core_test_case create proc conf error");
        return NULL;
    }

    pbcf->enable = NGX_CONF_UNSET;
    pbcf->port = NGX_CONF_UNSET_UINT;

    ngx_log_debug(NGX_LOG_DEBUG_ALL,cf->log, 0, "ngx_core_test_case_create_conf");

    return pbcf;
}


static char *
ngx_core_test_case_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_core_test_case_conf_t  *prev = parent;
    ngx_core_test_case_conf_t  *conf = child;

    ngx_conf_merge_uint_value(conf->port, prev->port, 0);
    ngx_conf_merge_off_value(conf->enable, prev->enable, 0);

    ngx_log_debug(NGX_LOG_DEBUG_ALL,cf->log, 0, "ngx_core_test_case_merge_conf");

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_core_test_case_prepare(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "core_test_case %V",
                  &ngx_cached_http_time);


    ngx_core_test_case_conf_t  *pbcf;

    pbcf = ngx_proc_get_conf(cycle->conf_ctx, ngx_core_test_case_module);
    if (!pbcf->enable) {
        return NGX_DECLINED;
    }

    if (pbcf->port == 0) {
        return NGX_DECLINED;
    }

    return NGX_OK;
}



static ngx_int_t
ngx_core_test_case_process_init(ngx_cycle_t *cycle)
{

    ngx_core_conf_t   *ccf;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);
    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "start test case");

    return test_main(0, ccf->environment, cycle );

}


static ngx_int_t
ngx_core_test_case_loop(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "core_test_case %V",
                  &ngx_cached_http_time);

    return NGX_OK;
}


static void
ngx_core_test_case_exit_process(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "core_test_case %V",
                  &ngx_cached_http_time);

}



