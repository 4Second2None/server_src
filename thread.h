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
 * connector
 */
typedef struct {
    LIBEVENT_THREAD *thread;
#define STATE_NOT_CONNECTED 0
#define STATE_CONNECTED 1
    volatile int state;
    int fd;
    struct sockaddr *sa;
    int socklen;
    struct event *timer;
    struct timeval tv;
    bufferevent *bev;
} connector;

void thread_init(int nthreads, struct event_base *base);
void dispatch_conn_new(int fd, char key, void *arg);
connector *connector_new(int fd, struct sockaddr *sa, int socklen);
void connector_free(connector *c);
void connector_write(connector *c, char *msg, size_t sz);

#endif /* THREAD_H_INCLUDED */
