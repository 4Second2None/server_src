#include "net.h"

void center_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        return;
    }

    conn_write(c, msg, sz);
}
