#include "thread.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void accept_cb(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
    printf("accept_cb\n");
    dispatch_fd_new(fd, 'c', NULL, 0);
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

struct connecting_timer_info {
    struct event *timer;
    struct timeval tv;
    int fd;
    char addr[16];
    short port;
};

static void go_connecting(int fd, short what, void *arg)
{
    struct connecting_timer_info *ti = (struct connecting_timer_info *)arg;

    dispatch_fd_new(ti->fd, 't', ti->addr, ti->port);

    free(ti);
}

void connecting_event_cb(struct bufferevent *bev, short what, void *arg)
{
    printf("connecting_event_cb what:%d\n", what);
    struct connecting_info *ci = (struct connecting_info *)arg;

    if (!(what & BEV_EVENT_CONNECTED)) {
        struct connecting_timer_info *ti = (struct connecting_timer_info *)malloc(sizeof(connecting_timer_info));
        if (NULL == ti) {
            fprintf(stderr, "connecting_timer_info alloc failed!\n");
            return;
        }

        ti->fd = ci->fd;
        strncpy(ti->addr, ci->addr, 16);
        ti->port = ci->port;
        ti->tv.tv_sec = 5;
        ti->tv.tv_usec = 0;
        ti->timer = evtimer_new(ci->thread->base, go_connecting, ti);
        evtimer_add(ti->timer, &ti->tv);
    }
    
    free(ci);
}
