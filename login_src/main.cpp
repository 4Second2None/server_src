#include "log.h"
#include "net.h"
#include "cmd.h"

#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

static void signal_cb(evutil_socket_t, short, void *);

/* rpc callback */
void client_rpc_cb(conn *, unsigned char *, size_t);
void center_rpc_cb(conn *, unsigned char *, size_t);

int main(int argc, char **argv)
{
    /* open log */
    if (0 != LOG_OPEN("./login", LOG_LEVEL_DEBUG, -1)) {
        return 1;
    }

    if (0 != check_cmd()) {
        return 1;
    }

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

    /* listener for client */
    struct sockaddr_in sa;
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(41000);

    listener *lc = listener_new(main_base, (struct sockaddr *)&sa, sizeof(sa), client_rpc_cb);
    if (NULL == lc) {
        fprintf(stderr, "create client listener failed!\n");
        return 1;
    }

    /* listener for client */
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(41001);

    listener *le = listener_new(main_base, (struct sockaddr *)&sa, sizeof(sa), center_rpc_cb);
    if (NULL == le) {
        fprintf(stderr, "create center listener failed!\n");
        return 1;
    }

    event_base_dispatch(main_base);

    listener_free(lc);
    listener_free(le);
    event_base_free(main_base);

    /* shutdown protobuf */
    google::protobuf::ShutdownProtobufLibrary();

    /* close log */
    LOG_CLOSE();

    return 0;
}

void signal_cb(evutil_socket_t fd, short what, void *arg)
{
    printf("signal_cb\n");
    struct event_base *base = (struct event_base *)arg;
    event_base_loopbreak(base);
}
