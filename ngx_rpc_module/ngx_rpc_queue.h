#ifndef __NGX_RPC_QUEUE_H_
#define __NGX_RPC_QUEUE_H_


#include <assert.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#define NGX_WRITE_FLAG 0x1u
#define NGX_READ_FLAG  0x2u
#define ngx_atomic_swap_set(x,y) __sync_lock_test_and_set(x, y)


// for cache line ?
typedef struct {
    ngx_atomic_t task; //only a pointer
    uint64_t pack[7];
} __attribute__((aligned(64))) task_elem_t;


/** muli-proudcer, muilt-custormer queue
 *
 */
typedef struct {
    task_elem_t *elems;
    uint64_t capacity;
    uint64_t process_num;
    uint64_t size; // TODO remove this field

    void(*notify)(void *, void *task);
    void(*wait)(void*, void *task);

    ngx_slab_pool_t *pool;
    ngx_log_t* log;

    __attribute__((aligned(64)))
    ngx_atomic_t readidx;

    __attribute__((aligned(64)))
    ngx_atomic_t writeidx;

} __attribute__((aligned(64))) ngx_rpc_queue_t;


///
/// \brief ngx_rpc_queue_create
/// \param shpool
/// \param max_elem
/// \return
///
ngx_rpc_queue_t *ngx_rpc_queue_create(ngx_slab_pool_t *shpool,int max_elem);


///
/// \brief ngx_rpc_queue_destory
/// \param queue
/// \return
///
int ngx_rpc_queue_destory(ngx_rpc_queue_t *queue);

///
/// \brief ngx_rpc_queue_push
/// \param queue
/// \param task
/// \return
///
int ngx_rpc_queue_push(ngx_rpc_queue_t *queue, void* task);

///
/// \brief ngx_rpc_queue_pop_block
/// \param queue
/// \param task
/// \param proc
///
void * ngx_rpc_queue_pop(ngx_rpc_queue_t *queue,void *proc);


#endif




