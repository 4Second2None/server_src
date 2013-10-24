#include "thread.h"

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/******************************* connection queue ********************************/

typedef struct conn_queue_item CQ_ITEM;
struct conn_queue_item {
    int fd;
    void *arg;
    CQ_ITEM *next;
};

typedef struct conn_queue CQ;
struct conn_queue {
    CQ_ITEM *head;
    CQ_ITEM *tail;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

static CQ_ITEM *cqi_freelist;
static pthread_mutex_t cqi_freelist_lock;

#define ITEMS_PER_ALLOC 16

static CQ_ITEM *cqi_new() {
    CQ_ITEM *item = NULL;
    pthread_mutex_lock(&cqi_freelist_lock);
    if (cqi_freelist) {
        item = cqi_freelist;
        cqi_freelist = item->next;
    }
    pthread_mutex_unlock(&cqi_freelist_lock);

    if (NULL == item) {
        int i;

        item = (CQ_ITEM *)malloc(sizeof(CQ_ITEM) * ITEMS_PER_ALLOC);
        if (NULL == item)
            return NULL;

        for (i = 2; i < ITEMS_PER_ALLOC; i++)
            item[i - 1].next = &item[i];

        pthread_mutex_lock(&cqi_freelist_lock);
        item[ITEMS_PER_ALLOC - 1].next = cqi_freelist;
        cqi_freelist = &item[i];
        pthread_mutex_unlock(&cqi_freelist_lock);
    }

    return item;
}

#undef ITEMS_PER_ALLOC

static void cqi_free(CQ_ITEM *item) {
    pthread_mutex_lock(&cqi_freelist_lock);
    item->next = cqi_freelist;
    cqi_freelist = item;
    pthread_mutex_unlock(&cqi_freelist_lock);
}

static void cq_init(CQ *cq) {
    pthread_mutex_init(&cq->lock, NULL);
    pthread_cond_init(&cq->cond, NULL);
    cq->head = NULL;
    cq->tail = NULL;
}

static void cq_push(CQ *cq, CQ_ITEM *item) {
    item->next = NULL;

    pthread_mutex_lock(&cq->lock);
    if (NULL == cq->tail)
        cq->head = item;
    else
        cq->tail->next = item;
    cq->tail = item;
    pthread_cond_signal(&cq->cond);
    pthread_mutex_unlock(&cq->lock);
}

static CQ_ITEM *cq_pop(CQ *cq) {
    CQ_ITEM *item;

    pthread_mutex_lock(&cq->lock);
    item = cq->head;
    if (NULL != item) {
        cq->head = item->next;
        if (NULL == cq->head)
            cq->tail = NULL;
    }
    pthread_mutex_unlock(&cq->lock);

    return item;
}

static LIBEVENT_DISPATCHER_THREAD dispatcher_thread;
static LIBEVENT_THREAD *threads;
static int num_threads;

/******************************* worker thread ********************************/

void conn_read_cb(struct bufferevent *, void *);
void conn_write_cb(struct bufferevent *, void *);
void conn_event_cb(struct bufferevent *, short, void *);
void connecting_event_cb(struct bufferevent *, short, void *);

static void thread_libevent_process(int fd, short which, void *arg)
{
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg;
    CQ_ITEM *item;
    char buf[1];

    if (read(fd, buf, 1) != 1)
        fprintf(stderr, "can't read from libevent pipe!\n");

    switch(buf[0]) {
        case 'c':
            {
                item = cq_pop(me->new_conn_queue);

                if (NULL != item) {
                    struct bufferevent* bev = bufferevent_socket_new(me->base, item->fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
                    if (NULL == bev) {
                        fprintf(stderr, "create bufferevent failed!\n");
                        close(item->fd);
                    }

                    evbuffer *input = bufferevent_get_input(bev);
                    evbuffer_enable_locking(input, NULL);
                    bufferevent_setcb(bev, conn_read_cb, conn_write_cb, conn_event_cb, me);
                    bufferevent_enable(bev, EV_READ);
                    printf("new connection established!\n");
                }
                cqi_free(item);
            }
            break;
        case 't':
            {
                item = cq_pop(me->new_conn_queue);

                if (NULL != item && NULL != item->arg) {
                    connector *c = (connector *)item->arg;
                    c->thread = me;
                    if (NULL == c->bev) {
                        c->bev = bufferevent_socket_new(me->base, item->fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
                        if (NULL == c->bev) {
                            fprintf(stderr, "create bufferevent failed!\n");
                            close(item->fd);
                        }
                    }

                    evbuffer *input = bufferevent_get_input(c->bev);
                    evbuffer_enable_locking(input, NULL);
                    bufferevent_setcb(c->bev, NULL, NULL, connecting_event_cb, c);
                    printf("connecting!\n");
                    bufferevent_socket_connect(c->bev, c->sa, c->socklen);
                }
                cqi_free(item);
            }
            break;
    }
}

static void setup_thread(LIBEVENT_THREAD *me) {
    me->base = event_base_new();
    if (NULL == me->base) {
        fprintf(stderr, "allocate event base failed!\n");
        exit(1);
    }

    event_set(&me->notify_event, me->notify_receive_fd,
            EV_READ | EV_PERSIST, thread_libevent_process, me);
    event_base_set(me->base, &me->notify_event);

    if (event_add(&me->notify_event, 0) == -1) {
        fprintf(stderr, "can't monitor libevent notify pipe!\n");
        exit(1);
    }

    me->new_conn_queue = (struct conn_queue *)malloc(sizeof(struct conn_queue));
    if (NULL == me->new_conn_queue) {
        fprintf(stderr, "connection queue alloc failed!\n");
        exit(EXIT_FAILURE);
    }
    cq_init(me->new_conn_queue);
}

static void create_worker(void *(*func)(void *), void *arg) {
    pthread_t thread;
    pthread_attr_t attr;
    int ret;

    pthread_attr_init(&attr);

    if ((ret = pthread_create(&thread, &attr, func, arg)) != 0) {
        fprintf(stderr, "pthread_create failed!\n");
        exit(1);
    }
}

static int init_count = 0;
static pthread_mutex_t init_lock;
static pthread_cond_t init_cond;

static void wait_for_thread_registration(int nthreads) {
    while (init_count < nthreads) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
}

static void register_thread_initialized() {
    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);
}

static void *worker_libevent(void *arg) {
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg;

    register_thread_initialized();
    event_base_dispatch(me->base);
    return NULL;
}

void thread_init(int nthreads, struct event_base *main_base)
{
    int i;

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

    pthread_mutex_init(&cqi_freelist_lock, NULL);
    cqi_freelist = NULL;

    dispatcher_thread.base = main_base;
    dispatcher_thread.thread_id = pthread_self();

    threads = (LIBEVENT_THREAD *)calloc(nthreads, sizeof(LIBEVENT_THREAD));
    if (NULL == threads) {
        fprintf(stderr, "allocate threads failed!");
        exit(1);
    }

    for (i = 0; i < nthreads; i++) {
        int fds[2];
        if (pipe(fds)) {
            fprintf(stderr, "can't create notify pipe!\n");
            exit(1);
        }

        threads[i].notify_receive_fd = fds[0];
        threads[i].notify_send_fd = fds[1];

        setup_thread(&threads[i]);
    }

    for (i = 0; i < nthreads; i++) {
        create_worker(worker_libevent, &threads[i]);
    }

    pthread_mutex_lock(&init_lock);
    wait_for_thread_registration(nthreads);
    pthread_mutex_unlock(&init_lock);

    num_threads = nthreads;
}

static int last_thread = -1;

void dispatch_conn_new(int fd, char key, void *arg) {
    CQ_ITEM *item = cqi_new();
    char buf[1];
    int tid = (last_thread + 1) % num_threads;

    LIBEVENT_THREAD *thread = threads + tid;

    last_thread = tid;

    item->fd = fd;
    item->arg = arg;
    
    cq_push(thread->new_conn_queue, item);

    buf[0] = key;
    if (write(thread->notify_send_fd, buf, 1) != 1) {
        fprintf(stderr, "writing to thread notify pipe failed!\n");
    }
}

/******************************* connector ********************************/

connector *connector_new(int fd, struct sockaddr *sa, int socklen)
{
    connector *c = (connector *)malloc(sizeof(connector));
    if (NULL == c) {
        fprintf(stderr, "connector alloc failed!\n");
        return NULL;
    }

    c->sa = (struct sockaddr *)malloc(socklen);
    if (NULL == c->sa) {
        fprintf(stderr, "sockaddr alloc failed!\n");
        free(c);
        return NULL;
    }

    memcpy(c->sa, sa, socklen);
    c->thread = NULL;
    c->fd = fd;
    c->socklen = socklen;
    c->timer = NULL;
    c->bev = NULL;
    return c;
}

void connector_free(connector *c)
{
    free(c->sa);
    bufferevent_free(c->bev);
    free(c);
}

int connector_write(connector *c, char *msg, size_t sz)
{
    if (c && c->state == STATE_CONNECTED && c->bev) {
        if (0 == bufferevent_write(c->bev, msg, sz))
            return bufferevent_enable(c->bev, EV_WRITE);
    }
    return -1;
}
