#include "ngx_rpc_buffer.h"



NgxChainBufferReader::NgxChainBufferReader(ngx_chain_t& ch):
    chain(&ch),
    totaly(0)
{
}

bool NgxChainBufferReader::Next(const void** data, int* size)
{
    ngx_buf_t *cur_buf = chain->buf;

    while(cur_buf && !cur_buf->in_file)
    {
        //cur is over
        if(cur_buf->pos == cur_buf->last)
        {

            if(chain->next != NULL)
            {
                 cur_buf = chain->next->buf;
                 continue;
            }else{

                return false;
            }
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
    if(chain->buf != NULL && chain->buf->last == chain->buf->end)
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
        *size = chain->buf->end - chain->buf->last;
        *data = chain->buf->last;

        chain->buf->pos = chain->buf->last;
        chain->buf->last = chain->buf->end;
    }else{
        // allocate new buff
        *size = default_size * extends;
        //chain->buf = ngx_create_temp_buf(pool, *size);

        ngx_buf_t* b =(ngx_buf_t*)ngx_palloc(pool, sizeof(ngx_buf_t));
        memset(b, 0, sizeof(ngx_buf_t));

        b->start = (u_char*)ngx_palloc(pool, *size);
        b->pos   = b->start;
        b->last  = b->start + *size;
        b->end   = b->last;
        b->temporary = 0;
        b->memory    = 1;

        extends += 1;
        *data = b->pos;

        chain->buf = b;
    }

    totaly += *size;
    return true;
}

void NgxChainBufferWriter::BackUp(int count)
{
    totaly -= count;
    chain->buf->last -= count;

    assert(chain->buf->last >= chain->buf->pos);
    assert(totaly>0);
}

////////////////////////////

NgxShmChainBufferWriter::NgxShmChainBufferWriter(ngx_chain_t& ch, ngx_slab_pool_t* p,unsigned int ex):
    chain(&ch),
    pool(p),
    totaly(0),
    extends(ex),
    default_size(4096)
{
    chain->buf = NULL;
    chain->next = NULL;
}

bool NgxShmChainBufferWriter::Next(void** data, int* size)
{

    // need allocate new chain
    if(chain->buf != NULL && chain->buf->last == chain->buf->end)
    {
        chain->next = (ngx_chain_t*)
                ngx_slab_alloc(pool, sizeof(ngx_chain_t));

        chain = chain->next;
        chain->next = NULL;
        chain->buf = NULL;
    }

    // if skiped has called
    if(chain->buf)
    {
        *size = chain->buf->end - chain->buf->last;
        *data = chain->buf->last;

        chain->buf->pos = chain->buf->last;
        chain->buf->last = chain->buf->end;

    }else{
        // allocate new buff
        *size = default_size * extends;
        //chain->buf = ngx_create_temp_buf(pool, *size);

        ngx_buf_t* b =(ngx_buf_t*)ngx_slab_alloc(pool, sizeof(ngx_buf_t));

        b->start = (u_char*)ngx_slab_alloc(pool, *size);
        b->pos   = b->start;
        b->last = b->start + *size;
        b->end   = b->last;

        b->temporary = 0;
        b->memory    = 1;

        *data = b->pos;

        chain->buf = b;
        extends += 1;
    }

    totaly += *size;
    return true;
}

void NgxShmChainBufferWriter::BackUp(int count)
{
    totaly -= count;
    chain->buf->last -= count;

    assert(chain->buf->last >= chain->buf->pos);
    assert(totaly>0);
}


