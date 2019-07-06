#include <cstring>		//for memset
#include "SocketManager.h"
#include "NetMsg.h"
#include "Socket.h"

using namespace hudp;

CSocketManager::CSocketManager() {

}

CSocketManager::~CSocketManager() {
    _socket_map.clear();
}

void CSocketManager::SendMsg(const HudpHandle& handle, NetMsg* msg) {
    auto ptr = GetSocket(handle);
    msg->_socket = ptr;
    ptr->SendMsgToPriQueue(msg);
}

void CSocketManager::Destory(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return;
    }

    // there should add msg to notify remote side destroy too.
    iter->second.reset();
}

std::shared_ptr<CSocket> CSocketManager::GetSocket(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter != _socket_map.end()) {
        return iter->second;
    }
    
//     std::shared_ptr<CSocket> socket = std::make_shared<CSocket>(new CSocket());
//     _socket_map[handle] = socket;
//     return socket;
}