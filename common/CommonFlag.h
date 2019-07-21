#ifndef HEADER_COMMON_COMMON
#define HEADER_COMMON_COMMON

#include <vector>
#include <string>
#include <functional>
#include "CommonType.h"
#include "HudpFlag.h"

namespace hudp {

    // processing phase
    // protocol resolution 
    const uint8_t     PP_PROTO_PARSE      = 1;
    // head handle
    const uint8_t     PP_HEAD_HANDLE      = 2;
    // body handle
    const uint8_t     PP_BODY_HANDLE      = 3;
    // upper program handle
    const uint8_t     PP_UPPER_HANDLE     = 4;

    // hudp use by itself
    enum hudp_proto_flag {
        // about set header field.
        // bag has incremental id.
        HPF_WITH_ID         = 0x0001,
        // bag has ack info. the ack is a range
        HPF_WITH_ACK_RANGE  = 0x0002,
        // bag has ack info. the ack is a array
        HPF_WITH_ACK_ARRAY  = 0x0004,
        // bag has body.
        HPF_WITH_BODY       = 0x0008,

        // 0x0010 ~ 0x0080 is priority

        // receiver need send back a ack.
        HPF_NEED_ACK        = 0x0100,
        // is orderly
        HPF_IS_ORDERLY      = 0x0200,
    };

    const uint16_t __mtu = 400;

}
#endif