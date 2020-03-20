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
        HPF_RELIABLE_ACK_RANGE         = 0x000100,
        // reliable orderly ack info is a range
        HPF_RELIABLE_ORDERLY_ACK_RANGE = 0x000200,

        // to close connection
        HPF_FIN                        = 0x000400,
        // close ack
        HPF_FIN_ACK                    = 0x000800,

        // bag has incremental id.
        HPF_WITH_ID                    = 0x001000,
        // bag has ack.
        HPF_WITH_RELIABLE_ACK          = 0x002000,
        HPF_WITH_RELIABLE_ORDERLY_ACK  = 0x004000,
        // bag has body.
        HPF_WITH_BODY                  = 0x008000,

        // msg with send time stamp, to calculation rtt time
        HPF_MSG_WITH_TIME_STAMP        = 0x010000,
    };

    /*
    * socket status when hudp running.
    * SS_INIT: when a socket just created.
    * SS_READY: client send a mag to remote and recv a ack back
    *           or server recv a msg. 
    *
    * The following states are the same as TCP.
    * SS_FIN_WAIT_1: when Close be called, send all msg in cache then send HPF_FIN to remote
    * SS_FIN_WAIT_2: when SS_FIN_WAIT_1 recv a HPF_FIN_ACK
    * SS_TIMW_WAIT: when SS_FIN_WAIT_2 recv a HPF_FIN
    * SS_CLOSE_WIAT: when SS_READY recv a HPF_FIN, send all msg in cache the send HPF_FIN to remote
    * SS_LAST_ACK: when SS_CLOSE_WIAT send HPF_FIN to remote until recv HPF_FIN_ACK
    *
    * status change:
    *     client         server
    *              MSG
    *     SS_INIT------->SS_INIT
    *       |               |
    *       V      ACK      V
    *    SS_READY<-------SS_READY
    *       
    *              FIN
    *    SS_READY------->SS_READY
    *       |               |
    *       V               V
    * SS_FIN_WAIT_1    SS_CLOSE_WIAT
    *       |
    *       V     FIN_ACK
    * SS_FIN_WAIT_2<---SS_CLOSE_WIAT
    *                       |
    *               FIN     V
    *  SS_TIME_WAIT<---SS_LAST_ACK
    *             FIN_ACK
    *  SS_TIME_WAIT--->SS_LAST_ACK
    */
    enum socket_status {
        SS_INIT       = 0,
        SS_READY      = 1,
        SS_FIN_WAIT_1 = 2,
        SS_FIN_WAIT_2 = 3,
        SS_TIME_WAIT  = 4,
        SS_CLOSE_WIAT = 5,
        SS_LAST_ACK   = 6
    };
}
#endif
