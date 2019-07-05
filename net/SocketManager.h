#ifndef HEADER_NET_SOCKETMANAGER
#define HEADER_NET_SOCKETMANAGER
#include <unordered_map>
#include <memory>
#include "CommonType.h"

namespace hudp {

    class CSocket;
    class CSocketManager {
    public:
        CSocketManager();
        ~CSocketManager();

        

    private:
       std::unordered_map<std::string, std::shared_ptr<CSocket>> _socket_map;
    };
}

#endif