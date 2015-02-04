#ifndef __NGX_LOG_CPP_H__
#define __NGX_LOG_CPP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#ifdef __cplusplus
}
#endif

#include <string>
#include <sstream>

#define LOG_DEBUG(log, p) \
      do { \
           std::stringstream str; \
           ngx_log_t* l = (ngx_log_t*)log; \
           str << p; \
           const std::string &s = str.str(); \
           ngx_log_debug(NGX_LOG_DEBUG_ALL, l, 0, "%s",s.c_str()); \
       } while(0);


#define LOG_ERROR(log, p) \
      do { \
           std::stringstream str; \
           str << p; \
           ngx_log_t* l = (ngx_log_t*)log; \
           const std::string &s = str.str(); \
           ngx_log_error(NGX_LOG_ERR, l, 0,"%s", s.c_str()); \
       } while(0);


#define LOG_WARN(log, p) \
      do { \
           std::stringstream str; \
           str << p; \
           ngx_log_t* l = (ngx_log_t*)log; \
           const std::string &s = str.str(); \
           ngx_log_error(NGX_LOG_ERR, l, 0, "%s", s.c_str()); \
       } while(0);



#endif

