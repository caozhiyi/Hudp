#ifndef HEADER_INCLUDE_HUDP
#define HEADER_INCLUDE_HUDP

#include "HudpFlag.h"

namespace hudp {

    // init library
    hudp_error_code Init();
    
    // start thread and recv with ip and port
    hudp_error_code Start(const std::string& ip, uint16_t port, const recv_back& recv_func, 
                          const can_write_back& can_write_func);

    hudp_error_code Join();

    // send msg
    hudp_error_code SendTo(const HudpHandle& handle, uint16_t flag, std::string& msg);
    hudp_error_code SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint32_t len);

    // destroy socket. release resources
    hudp_error_code Close(const HudpHandle& handle);

}

#endif
