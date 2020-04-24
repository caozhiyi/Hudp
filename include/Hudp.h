#ifndef HEADER_INCLUDE_HUDP
#define HEADER_INCLUDE_HUDP

#include "HudpFlag.h"

namespace hudp {

    // init library
    hudp_error_code Init();
    
    // start thread and recv with ip and port
    hudp_error_code Start(const std::string& ip, uint16_t port, const recv_back& recv_func, 
                          const send_back& write_func);

    // set connect call back function, not required
    void SetConnectCallBack(const connect_back& conn_func);

    // set resend call back function, not required
    void SetResendCallBack(const resend_back& resend_func);

    hudp_error_code Join();

    // send msg
    hudp_error_code SendTo(const HudpHandle& handle, uint16_t flag, std::string& msg, uint32_t upper_id = 0);
    hudp_error_code SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint32_t len, uint32_t upper_id = 0);

    // destroy socket. release resources
    hudp_error_code Close(const HudpHandle& handle);

}

#endif
