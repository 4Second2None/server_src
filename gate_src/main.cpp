#include "log.h"
#include "net.h"
#include "cmd.h"

#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

static void signal_cb(evutil_socket_t, short, void *);

/* callback */
void client_cb(conn *, unsigned char *, size_t);
void gate_cb(conn *, unsigned char *, size_t);

int main(int argc, char **argv)
{
    /* open log */
    if (0 != LOG_OPEN("./gate", LOG_LEVEL_DEBUG, -1)) {
        fprintf(stderr, "open gate log failed!\n");
        return 1;
    }

    if (0 != check_cmd()) {
        return 1;
    }

    /* protobuf verify version */
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct event_base *main_base = event_base_new();
    if (NULL == main_base) {
        mfatal("main_base = event_base_new() failed!");
        return 1;
    }

    conn_init();

    /* thread */
    thread_init(8, main_base);
    struct event *signal_event;

    /* signal */
    signal_event = evsignal_new(main_base, SIGINT, signal_cb, (void *)main_base);
    if (NULL == signal_event || 0 != event_add(signal_event, NULL)) {
        mfatal("create/add a signal event failed!");
        return 1;
    }

    /* listener for client */
    struct sockaddr_in sa;
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(42000);

    listener *lc = listener_new(main_base, (struct sockaddr *)&sa, sizeof(sa), client_cb);
    if (NULL == lc) {
        mfatal("create client listener failed!");
        return 1;
    }

    /* connector to center */
    struct sockaddr_in csa;
    bzero(&csa, sizeof(csa));
    csa.sin_family = AF_INET;
    csa.sin_addr.s_addr = inet_addr("127.0.0.1");
    csa.sin_port = htons(43000);

    connector *cg = connector_new((struct sockaddr *)&csa, sizeof(csa), gate_cb);
    if (NULL == cg) {
        mfatal("create center connector failed!");
        return 1;
    }

    event_base_dispatch(main_base);

    //connector_free(cg);
    listener_free(lc);
    event_base_free(main_base);

    /* shutdown protobuf */
    google::protobuf::ShutdownProtobufLibrary();

    /* close log */
    LOG_CLOSE();

    return 0;
}

void signal_cb(evutil_socket_t fd, short what, void *arg)
{
    mdebug("signal_cb");
    struct event_base *base = (struct event_base *)arg;
    event_base_loopbreak(base);
}
