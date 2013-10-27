#include "net.h"
#include "test.pb.h"

void center_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        return;
    }

    A a;
    msg_body<A>(msg, sz, &a);

    conn_write(c, msg, sz);
}
