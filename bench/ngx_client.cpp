#include "ngx_rpc_module/ngx_rpc_client_api.h"

//add a event fd to poll

//push a callback

//ngx_cycle
ngx_http_request_t * new_request(ngx_cycle_t *cycle)
{
      ngx_http_request_t * r = ngx_palloc(cycle->pool, sizeof(ngx_http_request_t));

      memset(r,0, sizeof(ngx_http_request_t));

      r->connection = ngx_get_connection(s, ev->log);



}




int main(int argc, char* argv[])
{
    printf("hehe");


    ngx_main(argc, argv);

    return 0;
}
