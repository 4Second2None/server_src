#include "net.h"
#include "test.pb.h"

void client_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        return;
    }

    A a;
    msg_body<A>(msg, sz, &a);
    printf("client_rpc_cb magic:%d len:%d cmd:%d, flags:%d A.info:%s\n", h.magic, h.len, h.cmd, h.flags, a.info().c_str());

    conn_write(c, msg, sz);
}
