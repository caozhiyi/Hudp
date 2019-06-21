#ifndef HEADER_COMMON
#define HEADER_COMMON

#include "CommonType.h"

enum proto_flag {
    // Transmission reliability control
    // only orderly. may lost some bag
    PF_ORDERLY          = 0x01,
    // only reliable. may be disorder
    PF_RELIABLE         = 0x02, 
    // reliable and orderly like tcp
    PF_RELIABLE_ORDERLY = 0x04, 
    // no other contral. only udp
    PF_NORMAL           = 0x08, 

    // about priority. Send two high-level packages and one low-level package when busy
    // the lowest priority.
    PF_LOW_PRI          = 0x10,
    // the normal priority.
    PF_NROMAL_PRI       = 0x20,
    // the high priority.
    PF_HIGH_PRI         = 0x40,
    // the highest priority.
    PF_HIGHEST_PRI      = 0x80,
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