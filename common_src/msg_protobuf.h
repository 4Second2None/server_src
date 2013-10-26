#ifndef MSG_PROTOBUF_H_INCLUDED
#define MSG_PROTOBUF_H_INCLUDED

#include "log.h"
#include "msg.h"

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>

int create_msg(uint16_t cmd, uint64_t uid, unsigned char **msg, size_t *sz);
int create_msg(uint16_t cmd, unsigned char **msg, size_t *sz);
int message_head(unsigned char *src, size_t src_sz, msg_head *h);

template<typename S>
int create_msg(uint16_t cmd, uint64_t uid, S *s, unsigned char **msg, size_t *sz)
{
    size_t body_sz = s->ByteSize();
    *sz = MSG_HEAD_SIZE + sizeof(uint64_t) + body_sz;
    *msg = (unsigned char *)malloc(*sz);
    if (NULL == *msg) {
        merror("msg alloc failed!");
        return -1;
    }

    unsigned short *cur = (unsigned short *)*msg;
    *cur++ = htons((unsigned short)MAGIC_NUMBER);
    *cur++ = htons((unsigned short)(sizeof(uint64_t) + body_sz));
    *cur++ = htons((unsigned short)cmd);
    *cur++ = htons((unsigned short)FLAG_HAS_UID);
    *((uint64_t *)cur)++ = htons((uint64_t)uid);
    
    google::protobuf::io::ArrayOutputStream arr(cur, body_sz);
    google::protobuf::io::CodedOutputStream output(&arr);
    s->SerializeToCodedStream(&output);
    return 0;
}

template<typename S>
int create_msg(uint16_t cmd, S *s, unsigned char **msg, size_t *sz)
{
    size_t body_sz = s->ByteSize();
    *sz = MSG_HEAD_SIZE + body_sz;
    *msg = (unsigned char *)malloc(*sz);
    if (NULL == *msg) {
        merror("msg alloc failed!");
        return -1;
    }

    unsigned short *cur = (unsigned short *)*msg;
    *cur++ = htons((unsigned short)MAGIC_NUMBER);
    *cur++ = htons((unsigned short)body_sz);
    *cur++ = htons((unsigned short)cmd);
    *cur++ = htons((unsigned short)0);
    
    google::protobuf::io::ArrayOutputStream arr(cur, body_sz);
    google::protobuf::io::CodedOutputStream output(&arr);
    s->SerializeToCodedStream(&output);
    return 0;
}


template<typename S>
int msg_body(unsigned char *src, size_t src_sz, uint64_t *uid, S *s)
{
    if (MSG_HEAD_SIZE > src_sz) {
        merror("msg less than head size!");
        return -1;
    }

    unsigned short *cur = (unsigned short *)src;

    cur++;
    unsigned short len = ntohs(*cur);
    if (MSG_HEAD_SIZE + len != src_sz) {
        merror("msg length error!");
        return -1;
    }

    cur += 2;
    unsigned short flag = ntohs(*cur);
    if (0 == flag & FLAG_HAS_UID) {
        merror(stderr, "msg no uid!");
        return -1;
    }

    cur++;
    google::protobuf::io::CodedInputStream input((const google::protobuf::uint8*)cur, len);
    s->ParseFromCodedStream(&input);
    return 0;
}

template<typename S>
int msg_body(unsigned char *src, size_t src_sz, S *s)
{
    if (MSG_HEAD_SIZE > src_sz) {
        merror("msg less than head size!");
        return -1;
    }
    unsigned short *cur = (unsigned short *)src;

    cur++;
    unsigned short len = ntohs(*cur);
    if (MSG_HEAD_SIZE + len != src_sz) {
        merror("msg length error!");
        return -1;
    }

    cur += 3;
    google::protobuf::io::CodedInputStream input((const google::protobuf::uint8*)cur, len);
    s->ParseFromCodedStream(&input);
    return 0;
}

#endif /* MSG_PROTOBUF_H_INCLUDED */
