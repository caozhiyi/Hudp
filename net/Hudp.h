#ifndef HEADER_NET_HUDP
#define HEADER_NET_HUDP

#include <functional>
#include <iostream>
#include <Single.h>
#include "CommonType.h"

namespace hudp {

    typedef std::string HudpHandle;
    typedef std::function<void(const HudpHandle& handlle, char* msg, uint16_t len)> recv_back;
    class CHudp : public base::CSingle<CHudp> {
    public:
        // initialize the hudp library. start thread
        void Init();

        // bind socket.
        bool Bind(uint16_t port);

        // send msg
        void SendTo(const HudpHandle& handlle, const std::string& msg);
        void SendTo(const HudpHandle& handlle, char* msg, uint16_t len);

        // set recvfrom call back function
        void SetRecvBack(const recv_back& func);

        // destory socket. release resources
        void Destroy(const HudpHandle& handlle);

    private:
        
    };

}

#endif