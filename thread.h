#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <event2/event.h>
#include <event.h>

#include <pthread.h>

struct conn_queue;

/*
 * worker
 */
typedef struct {
    pthread_t thread_id;
    struct event_base *base;
    struct event notify_event;
    int notify_receive_fd;
    int notify_send_fd;
    struct conn_queue *new_conn_queue;
} LIBEVENT_THREAD;

/*
 * master
 */
typedef struct {
    pthread_t thread_id;
    struct event_base *base;
} LIBEVENT_DISPATCHER_THREAD;

/*
 * connecting info
 */
struct connecting_info {
    LIBEVENT_THREAD *thread;
    int fd;
    char addr[16];
    short port;
};

void thread_init(int nthreads, struct event_base *base);
void dispatch_fd_new(int fd, char key, const char *addr, short port);

#endif /* THREAD_H_INCLUDED */
