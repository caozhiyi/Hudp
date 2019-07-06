#ifndef HEADER_NET_SOCKETMANAGER
#define HEADER_NET_SOCKETMANAGER
#include <unordered_map>
#include <memory>
#include "CommonType.h"
#include "CommonFlag.h"

namespace hudp {

    class CSocket;
    class NetMsg;
    class CSocketManager {
    public:
        CSocketManager();
        ~CSocketManager();

        void SendMsg(const HudpHandle& handle, NetMsg* msg);
        void Destory(const HudpHandle& handle);
        std::shared_ptr<CSocket> GetSocket(const HudpHandle& handle);

    private:
       std::unordered_map<std::string, std::shared_ptr<CSocket>> _socket_map;
    };
}

#endif