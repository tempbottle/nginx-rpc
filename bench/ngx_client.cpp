
#include <sys/eventfd.h>

#include "ngx_rpc_module/ngx_rpc_api.h"

static void *ngx_rpc_bench_create_main_conf(ngx_conf_t *cf);
static char *ngx_rpc_bench_init_main_conf(ngx_conf_t *cf, void *conf);
static ngx_int_t ngx_rpc_bench_process_init(ngx_cycle_t *cycle);
static void bench_main(void * param);


typedef struct {
    ngx_flag_t       enable;
} ngx_rpc_bench_conf_t;

typedef struct {
    const char* data;
} ngx_rpc_bench_ctx_t;


static ngx_command_t ngx_rpc_bench_commands[] = {

    { ngx_string("ngx_rpc_bench"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_rpc_bench_conf_t, enable),
      NULL },

    ngx_null_command
};


/* Modules */
static ngx_http_module_t  ngx_rpc_bench_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    ngx_rpc_bench_create_main_conf,   /* create main configuration */
    ngx_rpc_bench_init_main_conf,     /* init main configuration */

    NULL,                /* create server configuration */
    NULL,                /* merge server configuration */

    NULL,                /* create location configuration */
    NULL                 /* merge location configuration */
};

ngx_module_t  ngx_rpc_bench_module = {
    NGX_MODULE_V1,
    &ngx_rpc_bench_module_ctx,        /* module context */
    ngx_rpc_bench_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    /// there no where called init_master
    /// but some where called init module instead
    NULL,              /* init module */
    ngx_rpc_bench_process_init,             /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,             /* exit process */
    NULL,             /* exit master */
    NGX_MODULE_V1_PADDING
};


static void *ngx_rpc_bench_create_main_conf(ngx_conf_t *cf)
{

    ngx_rpc_bench_conf_t *conf = (ngx_rpc_bench_conf_t *)ngx_palloc(cf->pool, sizeof(ngx_rpc_bench_conf_t));

    conf->enable = NGX_CONF_UNSET;

    ngx_log_debug(NGX_LOG_DEBUG_ALL, cf->log, 0, "ngx_rpc_bench_create_main_conf");

    return conf;
}

static char *ngx_rpc_bench_init_main_conf(ngx_conf_t *cf, void *conf)
{

    ngx_rpc_bench_conf_t *c =(ngx_rpc_bench_conf_t *)conf;
    if(c->enable == NGX_CONF_UNSET)
    {
        c->enable = false;
    }

    return NGX_CONF_OK;

}
static ngx_int_t ngx_rpc_bench_process_init(ngx_cycle_t *cycle)
{

    ngx_rpc_bench_conf_t *conf = (ngx_rpc_bench_conf_t *)
            ngx_http_cycle_get_module_main_conf(cycle, ngx_rpc_bench_module);


    ngx_log_debug(NGX_LOG_DEBUG_ALL, cycle->log, 0, "ngx_rpc_bench_process_init enable:%d",conf->enable);
    if(conf->enable)
    {
        ngx_rpc_post_start.ctx = NULL;
        ngx_rpc_post_start.handler = bench_main;
    }
    return NGX_OK;
}



static void ngx_rpc_bench_parent_handler(ngx_http_request_t *r)
{
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "parenet request r->headers_out.status:%d", r->headers_out.status);

}

static ngx_int_t ngx_rpc_bench_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{

     ngx_http_request_t *pr = r->parent;

     pr->headers_out.status = r->headers_out.status;

     ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                 "subrequest r->headers_out.status:%d rc:%d", r->headers_out.status, rc);

     r->write_event_handler = ngx_rpc_bench_parent_handler;

     // the data in pr->out
    return NGX_OK;
}


void bench(void *param)
{
    ngx_log_debug(NGX_LOG_DEBUG_ALL, ngx_cycle->log, 0, "this from bench");

    int sockfd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);


    // int socket
    ngx_connection_t *c = ngx_get_connection(sockfd, ngx_cycle->log);
    c->pool = ngx_create_pool(1024, ngx_cycle->log);
    c->log = (ngx_log_t*)ngx_palloc(c->pool, sizeof(ngx_log_t));
    *(c->log) =  *(ngx_cycle->log);

    ngx_http_log_ctx_t *ctx = (ngx_http_log_ctx_t *)ngx_palloc(c->pool, sizeof(ngx_http_log_ctx_t));

    ctx->connection = c;
    ctx->request = NULL;
    ctx->current_request = NULL;

    c->log->connection = c->number;
    c->log->handler = NULL;
    c->log->data = ctx;
    c->log->action = (char*)"waiting for request";

    c->log_error = NGX_ERROR_INFO;

    c->pool->log = c->log;
    c->write->log = c->log;
    c->read->log = c->log;

    c->read->handler = ngx_http_empty_handler;
    c->write->handler = ngx_http_empty_handler;

    if( ngx_add_conn(c) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, c->log, 0, "ngx_add_conn  error");
    }

    ngx_http_connection_t *hc = (ngx_http_connection_t*)ngx_pcalloc(c->pool, sizeof(ngx_http_connection_t));


    ngx_http_core_main_conf_t*  core_main= (ngx_http_core_main_conf_t *)
            ngx_http_cycle_get_module_main_conf(ngx_cycle, ngx_http_core_module);

    ngx_http_core_srv_conf_t   **cscfp =(ngx_http_core_srv_conf_t **) core_main->servers.elts;

    hc->conf_ctx = cscfp[0]->ctx;
    c->data = hc;

    //ngx_http_init_connection(c);

    // prepare http_request
    ngx_reusable_connection(c, 0);
    ngx_http_request_t * r = ngx_http_create_request(c);
    r->main = r ;
    r->count ++;
    c->data  = r;


     // start subrequest
     ngx_http_post_subrequest_t *psr = (ngx_http_post_subrequest_t *)
             ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));

     psr->handler = ngx_rpc_bench_subrequest_post_handler;
     psr->data = NULL;
     r->request_body =  (ngx_http_request_body_t *)ngx_palloc(r->pool, sizeof(ngx_http_request_body_t));
     r->request_body->bufs = (ngx_chain_t*)ngx_palloc(r->pool, sizeof(ngx_chain_t));


     ngx_str_t path = ngx_string("/");

     ngx_http_request_t *sr;
     ngx_int_t rc = ngx_http_subrequest(r, &path, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);

     ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                 "ngx_http_subrequest rc:%d", rc);

     sr->write_event_handler(sr);
     //ngx_http_process_request(sr);
     //ngx_http_finalize_request(r, NGX_OK);
}

/// this must be called when moudule init done
void bench_main(void * param)
{

    ngx_rpc_bench_ctx_t ctx = {"hehe"};

    ngx_rpc_post_task(bench, &ctx);
}



