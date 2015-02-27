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


int _ngx_main(int argc, char *const *argv);

// add eventfd to trigger the nginx


#ifdef __cplusplus
}
#endif


#endif
