#ifndef HEADER_NET_SOCKETMANAGER
#define HEADER_NET_SOCKETMANAGER

#include <unordered_map>
#include <memory>
#include <list>
#include <condition_variable>
#include <mutex>

#include "CommonType.h"
#include "CommonFlag.h"
#include "Single.h"

namespace hudp {

    class CSocket;
    class NetMsg;
    class CSocketManager : public base::CSingle<CSocketManager> {
    public:
        CSocketManager();
        ~CSocketManager();
    
        // get a msg from priority queue.
        // all socket take msg out in turn.
        // if it's empty, it's blocked.
        NetMsg* GetMsg();

        // notify msg arrive 
        void NotifyMsg(const HudpHandle& handle);

        // get a socket.
        // if there isn't a socket, just create it.
        void GetSocket(const HudpHandle& handle, std::shared_ptr<CSocket>& socket);

        // destroy resource of socket and send destroy msg to remote.
        void Destroy(const HudpHandle& handle);

        // check socket exist
        bool Exist(const HudpHandle& handle);

    private:
        // get a socket from _socket_map. 
        // if there isn't a socket, just create it. 
        std::shared_ptr<CSocket> GetSocket(const HudpHandle& handle);

    private:
       std::unordered_map<HudpHandle, std::shared_ptr<CSocket>> _socket_map;

       std::mutex                   _mutex;
       std::condition_variable_any	_notify;
       std::list<HudpHandle>        _have_msg_socket;
    };
}

#endif