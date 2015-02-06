#include "ngx_rpc_buffer.h"


NgxChainBufferReader::NgxChainBufferReader(ngx_chain_t& ch):
    chain(&ch),
    cur_pos(NULL),
    totaly(0)
{
}

bool NgxChainBufferReader::Next(const void** data, int* size)
{
    ngx_buf_t *cur_buf = chain->buf;

    while(cur_buf && !cur_buf->in_file)
    {
        if(cur_buf->pos == cur_buf->last)
        {
            chain = chain->next;
            cur_buf = chain->buf;
            continue;
        }
        *data = cur_buf->pos;
        *size = cur_buf->last - cur_buf->pos;
        cur_buf->pos = cur_buf->last;
        totaly += *size;
        return true;
    }
    return false;
}

void NgxChainBufferReader::BackUp(int count)
{
    chain->buf->pos -= count;
}

bool NgxChainBufferReader::Skip(int count)
{

    ngx_buf_t *cur_buf = chain->buf;

    while(count >=0
          && cur_buf
          && !cur_buf->in_file)
    {
        int cur_size = cur_buf->last - cur_buf->pos;

        if(cur_size < count)
        {
            count  -= cur_size;
            totaly += cur_size;

            // move next
            chain = chain->next;
            cur_buf = chain ? NULL : chain->buf;
            continue;
        }

        cur_buf->pos += count;
        count = 0;
        totaly += count;
        return true;
    }

    return false;
}



NgxChainBufferWriter::NgxChainBufferWriter(ngx_chain_t& ch, ngx_pool_t* p,unsigned int ex):
    chain(&ch),
    pool(p),
    cur_pos(NULL),
    totaly(0),
    extends(ex),
    default_size(4096)
{
    chain->buf = NULL;
    chain->next = NULL;
}

bool NgxChainBufferWriter::Next(void** data, int* size)
{

    // need allocate new chain
    if(chain->buf != NULL && chain->buf->last == chain->buf->pos)
    {
        chain->next = (ngx_chain_t*)
                ngx_palloc(pool, sizeof(ngx_chain_t));

        chain = chain->next;
        chain->next = NULL;
        chain->buf = NULL;
    }

    // if skiped has called
    if(chain->buf)
    {
        *size = chain->buf->last - cur_pos;
        *data = cur_pos;
        cur_pos = chain->buf->last;
    }else{
        // allocate new buff
        *size = default_size * extends;
        chain->buf = ngx_create_temp_buf(pool, *size);
        cur_pos = chain->buf->last;

        extends += 1;
        *data = chain->buf->pos;
    }

    totaly += *size;
    return true;
}

void NgxChainBufferWriter::BackUp(int count)
{
    cur_pos -= count;
    totaly -= count;
}



