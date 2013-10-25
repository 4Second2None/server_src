#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <event2/event.h>
#include <event.h>

#include <pthread.h>

struct conn_queue;

typedef struct {
    pthread_t thread_id;
    struct event_base *base;
    struct event notify_event;
    int notify_receive_fd;
    int notify_send_fd;
    struct conn_queue *new_conn_queue;
} LIBEVENT_THREAD;

typedef struct {
    pthread_t thread_id;
    struct event_base *base;
} LIBEVENT_DISPATCHER_THREAD;

typedef struct conn conn;
struct conn {
    void *data;
    bufferevent *bev;
    LIBEVENT_THREAD *thread;
};

typedef struct connector connector;
struct connector {
#define STATE_NOT_CONNECTED 0
#define STATE_CONNECTED 1
    volatile int state;
    conn *c;
    struct sockaddr *sa;
    int socklen;
    /* reconnect timer */
    struct event *timer;
    struct timeval tv;
};

void thread_init(int nthreads, struct event_base *base);
void dispatch_conn_new(int fd, char key, void *arg);
conn *conn_new(int fd);
int conn_add_to_freelist(conn *c);
int conn_write(conn *c, unsigned char *msg, size_t sz);
connector *connector_new(struct sockaddr *sa, int socklen);
void connector_free(connector *cr);
int connector_write(connector *cr, unsigned char *msg, size_t sz);

#endif /* THREAD_H_INCLUDED */
