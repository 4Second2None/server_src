#include "net.h"

void gate_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        return;
    }

    conn_write(c, msg, sz);
}
