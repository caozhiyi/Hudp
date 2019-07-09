#ifndef HEADER_NET_SOCKETMANAGER
#define HEADER_NET_SOCKETMANAGER

#include <unordered_map>
#include <memory>
#include <list>
#include <condition_variable>
#include <mutex>

#include "CommonType.h"
#include "CommonFlag.h"

namespace hudp {

    class CSocket;
    class NetMsg;
    class CSocketManager {
    public:
        CSocketManager();
        ~CSocketManager();

        NetMsg* GetMsg();
        void SendMsg(const HudpHandle& handle, NetMsg* msg);
        void Destory(const HudpHandle& handle);
        std::shared_ptr<CSocket> GetSocket(const HudpHandle& handle);

    private:
       std::unordered_map<HudpHandle, std::shared_ptr<CSocket>> _socket_map;

       std::mutex                   _mutex;
       std::condition_variable_any	_notify;
       std::list<HudpHandle>        _have_msg_socket;
    };
}

#endif