#include "ngx_rpc_server_controller.h"

#include "ngx_rpc_buffer.h"
#include "ngx_log_cpp.h"

void NgxRpcServerController::FinishRequest(
        std::shared_ptr< NgxRpcServerController> cntl)
{
    // 1 save res to chain buffer
    ngx_chain_t chain;
    ngx_http_request_t * r = cntl->r;

    NgxChainBufferWriter writer(chain, r->pool);

    if(!cntl->res->SerializeToZeroCopyStream(&writer))
    {
        ERROR("SerializeToZeroCopyStream failed:"<<cntl->res->GetTypeName());
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR );
        return;
    }

    // 2 send header & body
    static ngx_str_t type = ngx_string(" application/x-protobuf");

    r->headers_out.content_type = type;
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = cntl->res->ByteSize();
    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;


    ngx_int_t rc = ngx_http_send_header(r);
    rc = ngx_http_output_filter(r, &chain);
    ngx_http_finalize_request(r, rc);
}



