#include "thread.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void accept_cb(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
    printf("accept_cb\n");
    dispatch_conn_new(fd, 'c', NULL);
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

static void go_connecting(int fd, short what, void *arg)
{
    connector *c = (connector *)arg;
    bufferevent_socket_connect(c->bev, c->sa, c->socklen);
}

static void delay_connecting(connector *c)
{
    if (NULL == c->timer) {
        c->tv.tv_sec = 5;
        c->tv.tv_usec = 0;
        c->timer = evtimer_new(c->thread->base, go_connecting, c);
        if (NULL == c->timer) {
            fprintf(stderr, "evtimer_new failed!\n");
            return;
        }
    }
    evtimer_add(c->timer, &c->tv);
}

void connecting_event_cb(struct bufferevent *bev, short what, void *arg)
{
    connector *c = (connector *)arg;

    if (!(what & BEV_EVENT_CONNECTED)) {
        printf("connecting failed!\n");
        delay_connecting(c);
    }
}
