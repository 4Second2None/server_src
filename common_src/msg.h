#ifndef MSG_H_INCLUDED
#define MSG_H_INCLUDED

typedef struct msg_head msg_head;
struct msg_head
{
#define MAGIC_NUMBER 0xabcd
    unsigned short magic;
    unsigned short len;
    unsigned short cmd;
#define FLAG_HAS_UID 1
    unsigned short flags;
};

#define MSG_HEAD_SIZE sizeof(unsigned short) * 4
#define MSG_MAX_SIZE  60 * 1024

#endif /* MSG_H_INCLUDED */
