#ifndef _NGX_RPC_CLIENT_API_H_
#define _NGX_RPC_CLIENT_API_H_


//this process some init function from nginx.c

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

// main function in nginx.c ,which be rename in libnginx.a
int _ngx_main(int argc, char *const *argv);

// add eventfd to trigger the nginx
// add a core moudule, nginx client,when worker init  add a event fd to epoll

#define NGX_CLIENT_MODULE          0x544E45494C43  /* "CLIENT" */
#define NGX_CLIENT_MAIN_CONF       0x02000000
#define NGX_CLIENT_CONF            0x04000000


#define NGX_CLIENT_MAIN_CONF_OFFSET  offsetof(ngx_client_conf_ctx_t, main_conf)
#define NGX_CLIENT_CONF_OFFSET       offsetof(ngx_client_conf_ctx_t, client_conf)


typedef struct {
    void                         **main_conf;
    void                         **client_conf;
} ngx_client_conf_ctx_t;


typedef struct {
    ngx_str_t                      name;

    ngx_int_t                      priority;
    ngx_msec_t                     delay_start;
    ngx_uint_t                     count;
    ngx_flag_t                     respawn;

    ngx_client_conf_ctx_t           *ctx;
} ngx_client_conf_t;


typedef struct {
    ngx_array_t                    processes; /* ngx_client_conf_t */
} ngx_client_main_conf_t;


typedef struct ngx_client_args_s {
    ngx_module_t                  *module;
    ngx_client_conf_t             *client_conf;
} ngx_client_args_t;


typedef struct {
    ngx_str_t                      name;
    void                        *(*create_main_conf)(ngx_conf_t *cf);
    char                        *(*init_main_conf)(ngx_conf_t *cf, void *conf);
    void                        *(*create_client_conf)(ngx_conf_t *cf);
    char                        *(*merge_client_conf)(ngx_conf_t *cf,
                                                    void *parent, void *child);

    ngx_int_t                    (*prepare)(ngx_cycle_t *cycle);
    ngx_int_t                    (*init)(ngx_cycle_t *cycle);
    ngx_int_t                    (*loop)(ngx_cycle_t *cycle);
    void                         (*exit)(ngx_cycle_t *cycle);
} ngx_client_module_t;


#define ngx_client_get_main_conf(conf_ctx, module)           \
    ((ngx_get_conf(conf_ctx, ngx_clients_module)) ?          \
        ((ngx_client_conf_ctx_t *) (ngx_get_conf(conf_ctx,   \
              ngx_clients_module)))->main_conf[module.ctx_index] : NULL)


#define ngx_client_get_conf(conf_ctx, module)                \
    ((ngx_get_conf(conf_ctx, ngx_clients_module)) ?          \
        ((ngx_client_conf_ctx_t *) (ngx_get_conf(conf_ctx,   \
              ngx_clients_module)))->client_conf[module.ctx_index] : NULL)


ngx_int_t ngx_procs_start(ngx_cycle_t *cycle, ngx_int_t type);


extern ngx_module_t  ngx_clients_module;
extern ngx_module_t  ngx_client_core_module;



#ifdef __cplusplus
}
#endif


#include <string>


int http_call(std::string& url, std::string& response)
{
    return 0;
}
#endif
