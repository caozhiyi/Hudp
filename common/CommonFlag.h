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
        HPF_WITH_ID                    = 0x0001,
        // bag has ack.
        HPF_WITH_RELIABLE_ACK          = 0x0002,
        HPF_WITH_RELIABLE_ORDERLY_ACK  = 0x0004,
        // bag has body.
        HPF_WITH_BODY                  = 0x0008,

        // reliable ack info is a range
        HPF_RELIABLE_ACK_RANGE         = 0x0100,
        // reliable orderly ack info is a range
        HPF_RELIABLE_ORDERLY_ACK_RANGE = 0x0200,

        // receiver need send back a ack.
        HPF_NEED_ACK                   = 0x0400,
        // is orderly
        HPF_IS_ORDERLY                 = 0x0800,

        // 0x0010 ~ 0x0080 is priority
    };

    const uint16_t __mtu = 400;

}
#endif