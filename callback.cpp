#include "thread.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <stdio.h>

void accept_cb(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
    printf("accept_cb\n");
    dispatch_fd_new(fd, 'c');
}

void conn_read_cb(struct bufferevent *bev, void *arg)
{
    printf("conn_read_cb\n");
}

void conn_write_cb(struct bufferevent *bev, void *arg)
{
    printf("conn_write_cb\n");
}

void conn_event_cb(struct bufferevent *bev, short what, void *arg)
{
    printf("conn_event_cb what:%d\n", what);
}

void connecting_event_cb(struct bufferevent *bev, short what, void *arg)
{
    printf("connecting_event_cb what:%d\n", what);
}
