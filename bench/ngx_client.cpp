#include "ngx_rpc_client_api.h"

int main(int argc,char* argv[])
{
    printf("hehe");

    ngx_cycle_t * cycle = NULL;
    if(0 != ngx_init_client(&cycle))
    {

        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "can not open pipes");
        return -1;
    }

    // event loop
    if (ngx_process == NGX_PROCESS_SINGLE)
    {
        ngx_single_process_cycle(cycle);
    } else {
        ngx_master_process_cycle(cycle);
    }

    return 0;
}
