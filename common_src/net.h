#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED

#include "msg_protobuf.h"

#include <event2/util.h>
#include <event2/listener.h>
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

typedef struct {
    void *data;
    void *user;
    bufferevent *bev;
    char addrtext[32];
    LIBEVENT_THREAD *thread;
} conn;

typedef void (*rpc_cb_func)(conn *, unsigned char *, size_t);

typedef struct {
    rpc_cb_func rpc;
} user_callback;

typedef struct {
    user_callback cb;
    evconnlistener *l;
    char addrtext[32];
} listener;

typedef struct {
    listener *l;
    char addrtext[32];
} listener_info;

typedef struct {
    user_callback cb;
#define STATE_NOT_CONNECTED 0
#define STATE_CONNECTED 1
    volatile int state;
    conn *c;
    struct sockaddr *sa;
    int socklen;
    char addrtext[32];
    /* reconnect timer */
    struct event *timer;
    struct timeval tv;
} connector;

/* thread clone */
void thread_init(struct event_base *base, int nthreads, pthread_t *th);

/* connection */
void conn_init();
void dispatch_conn_new(int fd, char key, void *arg);
conn *conn_new(int fd);
int conn_add_to_freelist(conn *c);
int conn_write(conn *c, unsigned char *msg, size_t sz);

/* listener */
listener *listener_new(struct event_base* base, struct sockaddr *sa, int socklen, rpc_cb_func rpc);
void listener_free(listener *l);

/* connector */
connector *connector_new(struct sockaddr *sa, int socklen, rpc_cb_func rpc);
void connector_free(connector *cr);
int connector_write(connector *cr, unsigned char *msg, size_t sz);

template<typename S>
int conn_write(conn *c, unsigned short cmd, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = conn_write(c, msg, sz);
    free(msg);
    return ret;
}

template<typename S>
int conn_write(conn *c, unsigned short cmd, uint64_t uid, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, uid, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = conn_write(c, msg, sz);
    free(msg);
    return ret;
}

int conn_write(conn *c, unsigned short cmd);
int conn_write(conn *c, unsigned short cmd, uint64_t uid);

template<typename S>
int connector_write(connector *cr, unsigned short cmd, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg<S>(cmd, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = connector_write(cr, msg, sz);
    free(msg);
    return ret;
}

template<typename S>
int connector_write(connector *cr, unsigned short cmd, uint64_t uid, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, uid, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = connector_write(cr, msg, sz);
    free(msg);
    return ret;
}

int connector_write(connector *cr, unsigned short cmd);
int connector_write(connector *cr, unsigned short cmd, uint64_t uid);

#endif /* NET_H_INCLUDED */
