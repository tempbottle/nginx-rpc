#include "ngx_log_cpp.h"

#include "google/protobuf/message.h"
ngx_log_t* cache_ngx_log = NULL;

static void LogHandler(google::protobuf::LogLevel level, const char* filename, int line,
                        const std::string& message)
{
     ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "file:%s line:%d message:%s",
                   filename, line, message.c_str());
}


ngx_log_t* set_cache_log( ngx_log_t* cache_log)
{
    ngx_log_t* pre = cache_ngx_log;
    cache_ngx_log = cache_log;

    google::protobuf::SetLogHandler(LogHandler);
    return pre;
}

ngx_log_t* get_cache_log()
{
    return cache_ngx_log ? cache_ngx_log : ngx_cycle->log;
}

