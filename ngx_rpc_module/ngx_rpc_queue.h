#ifndef __NGX_RPC_QUEUE_H_
#define __NGX_RPC_QUEUE_H_


#include <assert.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#include <ngx_rpc_task.h>


#define NGX_TASK_FLAG 0x1
#define NGX_NOTIFY_FLAG  0x2
#define ngx_atomic_swap_set(x,y) __sync_lock_test_and_set(x, y)



/** muli-proudcer, muilt-custormer queue
 *
 */
typedef struct {
    ngx_slab_pool_t *pool;
    ngx_log_t *log;

    // all idles proce ngx_rpc_processor_t
    ngx_queue_t      idles;
    ngx_shmtx_sh_t   idles_sh;
    ngx_shmtx_t      idles_lock;

    // it is diffcult to notify single pros or cons
    ngx_rpc_notify_t ** notify_slot;
    int notify_num;
    // the length is ngx_last_process

} ngx_rpc_queue_t;


///
/// \brief ngx_rpc_queue_create
/// \param shpool
/// \param max_elem
/// \return
///
ngx_rpc_queue_t *ngx_rpc_queue_create(ngx_slab_pool_t *shpool, ngx_log_t *log, int notify_num);
///
/// \brief ngx_rpc_queue_destory
/// \param queue
/// \return
///
int ngx_rpc_queue_destory(ngx_rpc_queue_t *queue);



////
/// \brief ngx_rpc_queue_init_per_process
/// \param queue
/// \param notify_ctx
/// \return
///
ngx_rpc_notify_t *ngx_rpc_queue_add_current_producer(ngx_rpc_queue_t *queue, void *ctx);
ngx_rpc_notify_t *ngx_rpc_queue_add_current_consumer(ngx_rpc_queue_t *queue, void *ctx);





int ngx_rpc_queue_push_and_notify(ngx_rpc_queue_t *queue, ngx_rpc_task_t *task);



#endif




