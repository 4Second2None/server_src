#include "thread.h"

void rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    conn_write(c, msg, sz);
}
