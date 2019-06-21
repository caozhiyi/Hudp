#ifndef HEADER_COMMON
#define HEADER_COMMON

#include "CommonType.h"

enum proto_flag {
    PF_NEED_ACK = 0x01,
    PF_
    
};

struct Header {
    uint8_t  _flag;
    uint16_t _id;
    uint16_t _len;
    uint16_t _ext_len;
};

struct Msg {
    Header _head;
    char*  _ext;
    char*  _body;
};

const uint16_t __header_len = sizeof(Header);
const uint16_t __mtu = 400;

#endif