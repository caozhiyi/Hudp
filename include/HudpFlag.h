#ifndef HEADER_INCLUDE_HUDPFLAG
#define HEADER_INCLUDE_HUDPFLAG

#include <functional>

namespace hudp {

    // Transmission reliability control
    enum hudp_tran_flag {
        // only orderly. may lost some bag
        HTF_ORDERLY          = 0x01,
        // only reliable. may be disorder
        HTF_RELIABLE         = 0x02,
        // reliable and orderly like tcp
        HTF_RELIABLE_ORDERLY = 0x04,
        // no other contral. only udp
        HTF_NORMAL           = 0x08
    };

    // about priority. Send two high-level packages and one low-level package when busy
    enum hudp_pri_flag {
        // the lowest priority.
        HPF_LOW_PRI          = 0x10,
        // the normal priority.
        HPF_NROMAL_PRI       = 0x20,
        // the high priority.
        HPF_HIGH_PRI         = 0x40,
        // the highest priority.
        HPF_HIGHEST_PRI      = 0x80
    };

    typedef std::string HudpHandle;
    typedef std::function<void(const HudpHandle& handle, const char* msg, uint32_t len)> recv_back;

}

#endif