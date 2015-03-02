#include "ngx_rpc_module/ngx_rpc_api.h"

typedef struct {
   const char * data;
} bench_ctx_t;

void bench(void *ctx)
{
   // bench_ctx_t * c = (bench_ctx_t *) ctx;
    printf("this from bench\n");
}


void ngx_bench_main(void * test){

    bench_ctx_t ctx = {"hehe"};
    ngx_rpc_post_task(bench, &ctx);
}

int main(int argc, char* argv[])
{
    printf("start bench:\n");

    ngx_rpc_post_start.ctx =  NULL;
    ngx_rpc_post_start.handler = ngx_bench_main;

    _ngx_main(argc, argv);

    return 0;
}
