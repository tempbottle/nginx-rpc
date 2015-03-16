#ifndef __NGX_RPC_SERVER_H_
#define __NGX_RPC_SERVER_H_

#include <memory>
#include <functional>

#include "google/protobuf/message.h"

#include "ngx_log_cpp.h"
#include "ngx_rpc_buffer.h"


int ngx_http_header_modify_content_length(ngx_http_request_t *r, ngx_int_t value)
{
    r->headers_in.content_length_n = value;


    ngx_list_part_t *part =  &r->headers_in.headers.part;
    ngx_table_elt_t *header = part->elts;
    unsigned int i = 0;
    for( i = 0; /* void */ ; i++)
    {
        if(i >= part->nelts)
        {
            if( part->next  == NULL)
            {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if(header[i].hash == 0)
        {
            continue;
        }

        if(0 == ngx_strncasecmp(header[i].key.data,
                                (u_char*)"Content-Length:",
                                header[i].key.len))
        {

            header[i].value.data = ngx_palloc(r->pool, 32);
            header[i].value.len = 32;

            snprintf((char*)header[i].value.data,
                     32, "%ld", value);
            r->headers_in.content_length->value =  header[i].value;

            return NGX_OK;
        }
    }
    return NGX_ERROR;
}

class RpcChannel;

typedef std::function <void(RpcChannel *channel,
                            const ::google::protobuf::Message*,
                            ::google::protobuf::Message*,
                            int)> RpcCallHandler;


class RpcChannel{
public:
    RpcChannel(ngx_http_request_t* _r)
        :r(_r)
    {
    }

    int setHeader()
    {
        return 0;
    }

    int getHeader()
    {
        return 0;
    }

    static void finish_request(RpcChannel *channel,
                               const ::google::protobuf::Message* req,
                               const ::google::protobuf::Message* res,
                               int status)
    {

        ngx_http_request_t * r = channel->r;

        static ngx_str_t type = ngx_string(" application/x-protobuf");
        r->headers_out.content_type = type;
        r->headers_out.status = status;
        ngx_int_t rc = NGX_HTTP_OK;

        if( status < NGX_HTTP_OK
                || status >= NGX_HTTP_SPECIAL_RESPONSE
                || res == NULL)
        {
            rc = ngx_http_send_header(r);
        }else{
            ngx_chain_t chain;
            NgxChainBufferWriter writer(chain, r->pool);

            if (!res->SerializeToZeroCopyStream(chain, r->pool))
            {
                ERROR("SerializeToZeroCopyStream failed:"
                      <<res->GetTypeName());
                ngx_http_finalize_request(r,
                                      NGX_HTTP_INTERNAL_SERVER_ERROR );
                return;
            }

            r->headers_out.content_length_n = writer.totaly;
            r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;

            rc = ngx_http_send_header(r);
            rc = ngx_http_output_filter(r, &chain);
        }

        ngx_http_finalize_request(r, rc);
    }

    static void parent_handler(ngx_http_request_t *r)
    {
        ngx_http_rpc_ctx_t* rpc_ctx =
                ngx_http_get_module_ctx(r, ngx_http_rpc_module);

        sub_request_ctx_t * sub_req_ctx  = (ngx_http_rpc_ctx_t*) rpc_ctx;

         r->write_event_handler = sub_req_ctx->pre_write_event_handler;

        sub_req_ctx->handler(sub_req_ctx->channel, sub_req_ctx->req,
                             sub_req_ctx->res, r->headers_out.status);
    }


    typedef struct {
        RpcChannel *channel;
        const ::google::protobuf::Message* req;
        ::google::protobuf::Message* res;
        RpcCallHandler handler;
        void (*pre_write_event_handler)(ngx_http_request_t *r);
    } sub_request_ctx_t;


    static void subrequest_post_handler(ngx_http_request_t *r,
                                                      void *data,
                                                      ngx_int_t rc)
    {

        ngx_http_request_t* child_req = r;
        ngx_http_request_t* parent_req = r->parent;
        parent_req->headers_out.status  = child_req->headers_out.status;

        sub_request_ctx_t * sub_req_ctx = (sub_request_ctx_t *)data;
        ngx_http_rpc_ctx_t* rpc_ctx =
                ngx_http_get_module_ctx(r, ngx_http_rpc_module);
        rpc_ctx ->sub_req_ctx = sub_req_ctx;

        ngx_http_upstream_t* rup = r->upstream;

        if(child_req->headers_out.status != NGX_HTTP_OK)
        {
             ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                         "subrequest failed,r->headers_out.status:%d rc:%d", r->headers_out.status, rc);
             pr->headers_out.status =  child_req->headers_out.status;

             sub_req_ctx->pre_write_event_handler = parent_req->write_event_handler;
             parent_req->write_event_handler = parent_handler;

             return;
        }

        NgxChainBufferReader reader(*rup->out_bufs, parent_req->pool);

        if(!sub_req_ctx->res->ParseFromZeroCopyStream(&reader))
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "subrequest failed,r->headers_out.status:%d rc:%d", r->headers_out.status, rc);
            pr->headers_out.status = NGX_HTTP_BAD_GATEWAY;
        }



        sub_req_ctx->pre_write_event_handler = parent_req->write_event_handler;
        parent_req->write_event_handler = parent_handler;
    }


    void sub_request(const std::string& path,
                     const ::google::protobuf::Message* req,
                     ::google::protobuf::Message* res,
                     RpcCallHandler handler)
    {



        ngx_http_post_subrequest_t *psr =
                ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));

        if(psr == NULL)
        {
            handler(this, req, res,NGX_HTTP_INTERNAL_SERVER_ERROR);
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR );
        }

        sub_request_ctx_t * ctx = new sub_request_ctx_t();
        ctx->channel = this;
        ctx->handler = handler;
        ctx->req     = req;
        ctx->res     = res;

        psr->handler = subrequest_post_handler;
        psr->data    = ctx;

        r->request_body->bufs = ngx_palloc(r->pool, sizeof(ngx_chain_t));

        NgxChainBufferWriter writer(*(r->request_body->bufs), r->pool);

        if(!req->SerializeToZeroCopyStream(&writer))
        {
            ERROR("SerializeToZeroCopyStream failed:"<<req->GetTypeName());
            done->Run();
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        ngx_http_header_modify_content_length(r, writer.totaly);

        ngx_str_t forward = ngx_string(path.c_str());

        ngx_http_request_t *sr;
        ngx_int_t rc = ngx_http_subrequest(r, &forward, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);

        if(rc != NGX_OK)
        {
            ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "ngx_http_subrequest failed:%d", rc);
            delete  ctx;
            handler(this, req, res, NGX_HTTP_INTERNAL_SERVER_ERROR);
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        }
    }



public:
    ngx_http_request_t* r;
};


class RpcAsynChannel  {


public:
      RpcChannel *channel;
};

#endif

