#include "net.h"
#include "msg.h"
#include "test.pb.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void accept_cb(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
    printf("accept_cb\n");
    dispatch_conn_new(fd, 'c', arg);
}

void conn_read_cb(struct bufferevent *bev, void *arg)
{
    size_t total_len;

    struct evbuffer* input = bufferevent_get_input(bev);
    total_len = evbuffer_get_length(input);
    printf("read_cb input_buffer_length:%zu\n", total_len);

    while (1)
    {
        if (total_len < MSG_HEAD_SIZE) {
            goto conti;
        }
        else {
            unsigned char *buffer;
            unsigned short *cur;
            unsigned short magic_number, len, cmd, flags;

            buffer = evbuffer_pullup(input, MSG_HEAD_SIZE);
            if (NULL == buffer)
            {
                fprintf(stderr, "evbuffer_pullup failed!\n");
                goto err;
            }

            cur = (unsigned short *)buffer;
            magic_number = ntohs(*(unsigned short *)cur++);
            if (MAGIC_NUMBER != magic_number)
            {
                fprintf(stderr, "magic_number error!\n");
                goto err;
            }

            len = ntohs(*(unsigned short *)cur++);
            printf("len:%d\n", len);

            if (MSG_MAX_SIZE < len)
            {
                fprintf(stderr, "len:%d > MSG_MAX_SIZE\n", len);
                goto err;
            }

            if (total_len < MSG_HEAD_SIZE + len)
                goto conti;

            cmd = ntohs(*(unsigned short *)cur++);
            flags = ntohs(*(unsigned short *)cur);
            printf("MESSAGE cmd:%d len:%d flags:%d\n", cmd, len, flags);

            size_t msg_len = MSG_HEAD_SIZE + len;
            buffer = evbuffer_pullup(input, msg_len);
            if (NULL == buffer)
            {
                fprintf(stderr, "evbuffer_pullup failed again!\n");
                goto err;
            }

            /* TODO frequency limit */

            /* callback */
            conn *c = (conn *)arg;
            user_callback *cb = (user_callback *)(c->data);
            if (cb->rpc)
                (*(cb->rpc))(c, buffer, msg_len);

            if (evbuffer_drain(input, msg_len) < 0)
            {
                fprintf(stderr, "evbuffer_drain failed!\n");
                goto err;
            }

            total_len -= msg_len;
        }
    }
    return;

err:
    printf("close sockect!\n");
    bufferevent_free(bev);
    return;
conti:
    bufferevent_enable(bev, EV_READ);
    return;
}

void conn_write_cb(struct bufferevent *bev, void *arg)
{
    printf("conn_write_cb\n");
    evbuffer *output = bufferevent_get_output(bev);
    size_t sz = evbuffer_get_length(output);
    printf("output length:%zu\n", sz);
    if (sz > 0)
        bufferevent_enable(bev, EV_WRITE);
}

void conn_event_cb(struct bufferevent *bev, short what, void *arg)
{
    printf("conn_event_cb what:%d\n", what);
}

/* used by connector */
static void conn_event_cb2(struct bufferevent *bev, short what, void *arg)
{
    printf("conn_event_cb2 what:%d\n", what);
}

static void go_connecting(int fd, short what, void *arg)
{
    conn *c = (conn *)arg;
    connector *cr = (connector *)c->data;
    bufferevent_socket_connect(c->bev, cr->sa, cr->socklen);
}

static void delay_connecting(conn *c)
{
    connector *cr = (connector *)c->data;
    if (NULL == cr->timer) {
        cr->tv.tv_sec = 5;
        cr->tv.tv_usec = 0;
        cr->timer = evtimer_new(c->thread->base, go_connecting, c);
        if (NULL == cr->timer) {
            fprintf(stderr, "evtimer_new failed!\n");
            return;
        }
    }
    evtimer_add(cr->timer, &cr->tv);
}

void connecting_event_cb(struct bufferevent *bev, short what, void *arg)
{
    conn *c = (conn *)arg;

    if (!(what & BEV_EVENT_CONNECTED)) {
        fprintf(stderr, "connecting failed!\n");
        delay_connecting(c);
    } else {
        printf("connect success!\n");
        connector *cr = (connector *)c->data;
        cr->state = STATE_CONNECTED;
        bufferevent_setcb(bev, conn_read_cb, conn_write_cb, conn_event_cb2, c);
        bufferevent_enable(bev, EV_READ);

        for (int i = 0; i < 40; i++)
        {
            A a;
            a.set_info("abc88888888888888888881222222222222223131321 hhhhhhhhhhhhhhfasdf fsfsf");
            connector_write<A>(cr, 1, &a);
        }
    }
}
