#ifndef HEADER_COMMON_COMMON
#define HEADER_COMMON_COMMON

#include <vector>
#include <string>
#include <functional>

#include "HudpFlag.h"
#include "CommonType.h"

namespace hudp {

    // hudp use by itself
    enum hudp_proto_flag {
        // 0x0001 ~ 0x0008 is reliable
        // 0x0010 ~ 0x0080 is priority

        // reliable ack info is a range
        HPF_RELIABLE_ACK_RANGE         = 0x0100,
        // reliable orderly ack info is a range
        HPF_RELIABLE_ORDERLY_ACK_RANGE = 0x0200,

        // to close connection
        HPF_CLOSE                      = 0x0400,
        // close ack
        HPF_CLOSE_ACK                  = 0x0800,

        // bag has incremental id.
        HPF_WITH_ID                    = 0x1000,
        // bag has ack.
        HPF_WITH_RELIABLE_ACK          = 0x2000,
        HPF_WITH_RELIABLE_ORDERLY_ACK  = 0x4000,
        // bag has body.
        HPF_WITH_BODY                  = 0x8000,
    };
}
#endif
