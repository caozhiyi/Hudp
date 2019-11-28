#ifndef HEADER_NET_SOCKETMANAGER
#define HEADER_NET_SOCKETMANAGER

#include <unordered_map>
#include <memory>
#include <list>
#include <condition_variable>
#include <mutex>

#include "CommonType.h"
#include "CommonFlag.h"
#include "ISocketManager.h"

namespace hudp {

    class CSocket;
    // not thread safe
    class CSocketManagerImpl : public CSocketManager {
    public:
        CSocketManagerImpl();
        ~CSocketManagerImpl();
    
        bool IsSocketExist(const HudpHandle& handle);
        // return a socket, create one if not exist.
        std::shared_ptr<CSocket> GetSocket(const HudpHandle& handle);
        // remove a socket directly
        void DeleteSocket(const HudpHandle& handle);
        // send close msg to remote
        void CloseSocket(const HudpHandle& handle);

        // get a socket.
        // if there isn't a socket, just create it.
        void GetSocket(const HudpHandle& handle, std::shared_ptr<CSocket>& socket);

    private:
        // get a socket from _socket_map. 
        // if there isn't a socket, just create it. 
        std::shared_ptr<CSocket> GetSocket(const HudpHandle& handle);

    private:
        std::unordered_map<HudpHandle, std::shared_ptr<CSocket>> _socket_map;
    };
}

#endif