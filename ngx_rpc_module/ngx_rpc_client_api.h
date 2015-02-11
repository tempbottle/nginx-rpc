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
#include <nginx.h>

int ngx_main(int argc, char *const *argv);


#ifdef __cplusplus
}
#endif


#endif
