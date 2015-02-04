#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#include "ngx_rpc_api.h"

// for content handler
void* ngx_http_soap_sub_create_srv_conf(ngx_conf_t *cf);
ngx_int_t ngx_http_rpc_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_rpc_preconfiguration(ngx_conf_t *cf);
ngx_int_t ngx_http_rpc_postconfiguration(ngx_conf_t *cf);

// for filter
static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;
static ngx_int_t ngx_http_filter_rpc_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_filter_rpc_body_filter(ngx_http_request_t *r, ngx_chain_t* in);


/* Commands */
static ngx_command_t  ngx_http_rpc_module_commands[] = {
    { ngx_string("ngx_rpc_conf_file"),
      NGX_HTTP_SRV_CONF | NGX_CONF_NOARGS,
      ngx_conf_set_path_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_rpc_module_conf_t, json_conf_path),
      NULL },
    ngx_null_command
};

/* Modules */
static ngx_http_module_t  ngx_http_rpc_module_ctx = {
    ngx_http_rpc_preconfiguration, /* preconfiguration */
    ngx_http_rpc_postconfiguration, /* postconfiguration */

    NULL,                /* create main configuration */
    NULL,                /* init main configuration */

    NULL,                /* create server configuration */
    NULL,                /* merge server configuration */

    NULL,                /* create location configuration */
    NULL                 /* merge location configuration */
};


ngx_module_t  ngx_http_rpc_module = {
    NGX_MODULE_V1,
    &ngx_http_rpc_module_ctx,              /* module context */
    ngx_http_rpc_module_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but some where called init module instead
    ngx_http_rpc_master_init,              /* init module */
    ngx_http_rpc_process_init,             /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_http_rpc_process_exit,             /* exit process */
    ngx_http_rpc_master_exit,             /* exit master */
    NGX_MODULE_V1_PADDING
};


///
/// \brief ngx_http_rpc_preconfiguration
///        add the ngx_http_rpc_handler
/// \param cf
/// \return
///
ngx_int_t ngx_http_rpc_preconfiguration(ngx_conf_t *cf)
{
    ngx_http_handler_pt *h = NULL;

    ngx_http_core_main_conf_t *cmcf =
            ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);

    *h = ngx_http_rpc_handler;

    return 0;
}


ngx_int_t ngx_http_rpc_postconfiguration(ngx_conf_t *cf){

    ngx_http_next_header_filter =
            ngx_http_top_header_filter;

    ngx_http_top_header_filter =
            ngx_http_filter_rpc_header_filter;

    ngx_http_next_body_filter =
            ngx_http_top_body_filter;

    ngx_http_top_body_filter =
            ngx_http_filter_rpc_body_filter;

    return NGX_OK;
}


void* ngx_http_soap_sub_create_srv_conf(ngx_conf_t *cf)
{
    ngx_http_rpc_conf_t *conf = (ngx_http_rpc_conf_t *)ngx_pcalloc(
                cf->pool,
                sizeof(ngx_http_rpc_conf_t));

    if(conf == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, cf->log,
                      errno, "ngx_pcalloc failed");
        return NULL;
    }

    conf->json_conf_path.data = NULL;
    conf->json_conf_path.len  = 0;

    return conf;
}

/** just foward process the body */
ngx_int_t ngx_http_rpc_handler(ngx_http_request_t *r)
{
    ngx_int_t rc = ngx_http_read_client_request_body(r, ngx_http_rpc_post_hander);

    if(rc >=  NGX_HTTP_SPECIAL_RESPONSE)
    {
        ngx_log_error(NGX_LOG_WARN,
                      r->connection->log,
                      0,
                      "Method:%s,url:%V is not allowed for rpc moudle!",
                      r->request_start,
                      &(r->uri));
        return rc;
    }

    return NGX_OK;
}

ngx_int_t ngx_http_filter_rpc_header_filter(ngx_http_request_t *r){

    ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log
                  , 0, "ngx_http_filter_rpc_header_filter ");

    return ngx_http_next_header_filter(r);
}

ngx_int_t ngx_http_filter_rpc_body_filter(ngx_http_request_t *r,
                               ngx_chain_t* in){

    ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log
                  , 0, "ngx_http_filter_rpc_body_filter%V");

    return ngx_http_next_body_filter(r, in);
}



