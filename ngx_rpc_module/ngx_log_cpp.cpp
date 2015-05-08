#include "ngx_log_cpp.h"

ngx_log_t* cache_ngx_log = NULL;


ngx_log_t* set_cache_log( ngx_log_t* cache_log)
{
    ngx_log_t* pre = cache_ngx_log;
    cache_ngx_log = cache_log;
    return pre;
}

ngx_log_t* get_cache_log()
{
    return cache_ngx_log ? cache_ngx_log : ngx_cycle->log;
}

