#include "thread.h"

#include <google/protobuf/stubs/common.h>

#include <event2/listener.h>
#include <event2/util.h>

#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

void conn_init();
static void signal_cb(evutil_socket_t, short, void *);
void accept_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);

int main(int argc, char **argv)
{
    /* protobuf verify version */
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct event_base *main_base = event_base_new();
    if (NULL == main_base) {
        fprintf(stderr, "main_base = event_base_new() failed!\n");
        return 1;
    }

    conn_init();

    /* thread */
    thread_init(8, main_base);
    struct event *signal_event;

    /* signal */
    signal_event = evsignal_new(main_base, SIGINT, signal_cb, (void *)main_base);
    if (NULL == signal_event || 0 != event_add(signal_event, NULL)) {
        fprintf(stderr, "create/add a signal event failed!\n");
        return 1;
    }

    /* listener */
    struct evconnlistener *listener;
    struct sockaddr_in sa;
    short port = 5555;

    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);

    listener = evconnlistener_new_bind(main_base, accept_cb, (void *)main_base,
            LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
            (struct sockaddr *)&sa, sizeof(sa));

    if (NULL == listener) {
        fprintf(stderr, "create listener failed!\n");
        return 1;
    }

    /* connector to center */
    struct sockaddr_in csa;
    bzero(&csa, sizeof(csa));
    csa.sin_family = AF_INET;
    csa.sin_addr.s_addr = inet_addr("127.0.0.1");
    csa.sin_port = htons(5555);

    connector *center = connector_new((struct sockaddr *)&csa, sizeof(csa));
    if (center) {
        dispatch_conn_new(-1, 't', center);
    }

    event_base_dispatch(main_base);

    connector_free(center);
    event_base_free(main_base);

    /* shutdown protobuf */
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

void signal_cb(evutil_socket_t fd, short what, void *arg)
{
    printf("signal_cb\n");
    struct event_base *base = (struct event_base *)arg;
    event_base_loopbreak(base);
}
