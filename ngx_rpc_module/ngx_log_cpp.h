#ifndef __NGX_LOG_CPP_H__
#define __NGX_LOG_CPP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_core.h>
#include <ngx_log.h>


extern ngx_log_t* cache_ngx_log;

ngx_log_t* set_cache_log( ngx_log_t* cache_log) ;

ngx_log_t* get_cache_log();

#ifdef __cplusplus
}
#endif

#include <string>
#include <sstream>

#define DEBUG(p) \
      do { \
           std::stringstream str; \
           str << p; \
           const std::string &s = str.str(); \
           ngx_log_debug(NGX_LOG_DEBUG_ALL, get_cache_log(), 0, "%s",s.c_str()); \
       } while(0);

#define INFO(p) \
      do { \
           std::stringstream str; \
           str << p; \
           const std::string &s = str.str(); \
           ngx_log_error(NGX_LOG_INFO, get_cache_log(), 0, "%s", s.c_str()); \
       } while(0);

#define WARN(p) \
      do { \
           std::stringstream str; \
           str << p; \
           const std::string &s = str.str(); \
           ngx_log_error(NGX_LOG_ERR, get_cache_log(), 0, "%s", s.c_str()); \
       } while(0);

#define ERROR(p) \
      do { \
           std::stringstream str; \
           str << p; \
           const std::string &s = str.str(); \
           ngx_log_error(NGX_LOG_ERR, get_cache_log(), 0,"%s", s.c_str()); \
       } while(0);

#endif

