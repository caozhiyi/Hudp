#ifndef HEADER_INCLUDE_HUDP
#define HEADER_INCLUDE_HUDP

#include "HudpFlag.h"

namespace hudp {

    // init library
    void Init();
    
    // start thread and recv with port and
    // the first IP on a network card that is not a loopback
    bool Start(uint16_t port, const recv_back& func);
    // start thread and recv with ip and port
    bool Start(const std::string& ip,uint16_t port, const recv_back& func);

    void Join();

    // send msg
    void SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg);
    void SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint16_t len);

    // destory socket. release resources
    void Close(const HudpHandle& handle);

}

#endif
