#ifndef HEADER_INCLUDE_HUDPFLAG
#define HEADER_INCLUDE_HUDPFLAG

#include <functional>

namespace hudp {

    // Transmission reliability control
    enum hudp_tran_flag {
        // only orderly. may lost some bag
        HTF_ORDERLY          = 0x0001,
        // only reliable. may be disorder
        HTF_RELIABLE         = 0x0002,
        // reliable and orderly like tcp
        HTF_RELIABLE_ORDERLY = 0x0004,
        // no other contral. only udp
        HTF_NORMAL           = 0x0008
    };

    // about priority. Send two high-level packages and one low-level package when busy
    enum hudp_pri_flag {
        // the lowest priority.
        HPF_LOW_PRI          = 0x0010,
        // the normal priority.
        HPF_NROMAL_PRI       = 0x0020,
        // the high priority.
        HPF_HIGH_PRI         = 0x0040,
        // the highest priority.
        HPF_HIGHEST_PRI      = 0x0080
    };

    typedef std::string HudpHandle;
    typedef std::function<void(const HudpHandle& handlle, const char* msg, uint16_t len)> recv_back;

}

#endif