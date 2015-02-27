#include "ngx_rpc_client_api.h"

#define NGX_CLIENT_MODULE          0x544E45494C43  /* "CLIENT" */
#define NGX_CLIENT_MAIN_CONF       0x02000000
#define NGX_CLIENT_CONF            0x04000000


#define NGX_PROC_MAIN_CONF_OFFSET  offsetof(ngx_proc_conf_ctx_t, main_conf)
#define NGX_PROC_CONF_OFFSET       offsetof(ngx_proc_conf_ctx_t, proc_conf)


#ifdef __cplusplus
extern "C" {
#endif




#ifdef __cplusplus
}
#endif
