#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

#define NGX_WRITE_FLAG 0x1u
#define NGX_READ_FLAG  0x2u
#define ngx_atomic_swap_set(x,y) __sync_lock_test_and_set(x, y)

typedef struct {
    ngx_atomic_t task;
    uint64_t pack[7];
} __attribute__((aligned(64))) task_elem_t;


typedef struct {
    task_elem_t *elems;
    uint64_t capacity;
    uint64_t process_num;
    uint64_t size;
    void(*process)(void * ,void *task);
    void(*wait)(void*,void *task);
    ngx_log_t *log;
    ngx_slab_pool_t * pool;

    __attribute__((aligned(64)))
    ngx_atomic_t readidx;

    __attribute__((aligned(64)))
    ngx_atomic_t writeidx;

} __attribute__((aligned(64))) ngx_rpc_queue_t;


///
int ngx_rpc_queue_create(ngx_rpc_queue_t **queue, ngx_cycle_t * cycle, ngx_slab_pool_t * shpool);

int ngx_rpc_queue_destory(ngx_rpc_queue_t *queue);


bool ngx_rpc_push_task(ngx_rpc_queue_t *queue, void* task, void *proc);


void  ngx_rpc_pop_task_block(ngx_rpc_queue_t *queue, void** task, void *proc);

////////////////////////////////////////////////////////////////////////////////
typedef struct {
     void(*hanlder)(void*);
     void* ctx;
     ngx_queue_t next;

} ngx_rpc_notify_task_t;



typedef struct {

    ngx_connection_t* notify_conn;

    int eventf_fd;
    ngx_queue_t task;
    ngx_shmtx_t lock_task;

    void(*read_hanlder)(void*);
    void(*write_hanlder)(void*);
    void *ctx;
    ngx_slab_pool_t *shpool;

} ngx_rpc_notify_t;



void ngx_rpc_nofiy_default_hanlder(void*);

int ngx_rpc_nofiy_read_handler(ngx_event_t *ev);

static void ngx_event_hanlder_notify_write(ngx_event_t *ev);


int ngx_rpc_notify_create(ngx_rpc_notify_t* notify, ngx_cycle_t *cycle, ngx_slab_pool_t *shpool , void *ctx);
int ngx_rpc_notify_destory(ngx_rpc_notify_t* notify);
int ngx_rpc_notify_task(ngx_rpc_notify_t* notify, void(*hanlder)(void*), void* data);

///////














#ifdef __cplusplus
}
#endif
