#ifndef HEADER_INCLUDE_HUDP
#define HEADER_INCLUDE_HUDP

#include "HudpFlag.h"

namespace hudp {

    class CHudp {
    public:

        // init library
        static void Init(bool log = false);
        // start thread and recv
        static bool Start(uint16_t port, const recv_back& func);

        static void Join();

        // send msg
        static void SendTo(const HudpHandle& handlle, uint16_t flag, const std::string& msg);
        static void SendTo(const HudpHandle& handlle, uint16_t flag, const char* msg, uint16_t len);

        // destory socket. release resources
        static void Destroy(const HudpHandle& handlle);
    };

}

#endif