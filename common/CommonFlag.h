#ifndef HEADER_COMMON_COMMON
#define HEADER_COMMON_COMMON

#include <vector>
#include <string>
#include <functional>
#include "CommonType.h"
namespace hudp {

    // processing phase
    enum process_phase {
        // protocol resolution 
        PP_PROTO_PARSE      = 0x0001,
        // head handle
        PP_HEAD_HANDLE      = 0x0002,
        // body handle
        PP_BODY_HANDLE      = 0x0004,
        // upper program handle
        PP_UPPER_HANDLE     = 0x0008
    };

    // Transmission reliability control
    enum hudp_tran_flag {
        // only orderly. may lost some bag
        HTF_ORDERLY         = 0x0001,
        // only reliable. may be disorder
        HTF_RELIABLE        = 0x0002,
        // reliable and orderly like tcp
        HTF_RELIABLE_ORDERLY= 0x0004,
        // no other contral. only udp
        HTF_NORMAL          = 0x0008
    };
    
    // about priority. Send two high-level packages and one low-level package when busy
    enum hudp_pri_flag {
        // the lowest priority.
        HPF_LOW_PRI         = 0x0010,
        // the normal priority.
        HPF_NROMAL_PRI      = 0x0020,
        // the high priority.
        HPF_HIGH_PRI        = 0x0040,
        // the highest priority.
        HPF_HIGHEST_PRI     = 0x0080
    };

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
        // 
    };

    typedef std::string HudpHandle;
    typedef std::function<void(const HudpHandle& handlle, const char* msg, uint16_t len)> recv_back;

    const uint16_t __mtu = 400;

}
#endif