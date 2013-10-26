#include "net.h"

void client_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        return;
    }

    printf("client_rpc_cb magic:%d len:%d cmd:%d, flags:%d\n", h.magic, h.len, h.cmd, h.flags);

    conn_write(c, msg, sz);
}
