#ifndef _NGX_RPC_BUFFER_H_
#define _NGX_RPC_BUFFER_H_

extern "C" {
#include "ngx_core.h"
}

#include "google/protobuf/message.h"
#include "google/protobuf/io/zero_copy_stream.h"

class NgxChainBufferReader :
        public ::google::protobuf::io::ZeroCopyInputStream
{
public:

    NgxChainBufferReader(ngx_chain_t& ch);

    virtual bool Next(const void** data, int* size) ;

    virtual void BackUp(int count);

    virtual bool Skip(int count);

    virtual ::google::protobuf::int64 ByteCount() const { return totaly;}

public:
    ngx_chain_t *chain;
    unsigned int totaly;
};


class NgxChainBufferWriter :
        public ::google::protobuf::io::ZeroCopyOutputStream
{
public:
    NgxChainBufferWriter(ngx_chain_t& ch, ngx_pool_t* p, unsigned int ex = 1);

    virtual bool Next(void** data, int* size);

    virtual void BackUp(int count);

    virtual ::google::protobuf::int64 ByteCount() const { return totaly;}

public:
    ngx_chain_t *chain;
    ngx_pool_t* pool;

    int totaly;

    unsigned int extends;
    unsigned int default_size;

};


class NgxShmChainBufferWriter :
        public ::google::protobuf::io::ZeroCopyOutputStream
{
public:
    NgxShmChainBufferWriter(ngx_chain_t& ch, ngx_slab_pool_t *p, unsigned int ex = 1);

    virtual bool Next(void** data, int* size);

    virtual void BackUp(int count);

    virtual ::google::protobuf::int64 ByteCount() const { return totaly;}

public:
    ngx_chain_t *chain;
    ngx_slab_pool_t* pool;

    int totaly;

    unsigned int extends;
    unsigned int default_size;

};



#endif
