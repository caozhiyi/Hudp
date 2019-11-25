#include "SocketManager.h"
#include "Socket.h"
#include "HudpImpl.h"

using namespace hudp;

CSocketManagerImpl::CSocketManagerImpl() {

}

CSocketManagerImpl::~CSocketManagerImpl() {
    _socket_map.clear();
}

bool CSocketManagerImpl::IsSocketExist(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return false;
    }
    return true;
}

std::shared_ptr<CSocket> CSocketManagerImpl::GetSocket(const HudpHandle& handle) {
    return GetSocket(handle);
}

void CSocketManagerImpl::DeleteSocket(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return;
    }

    iter->second.reset();
    _socket_map.erase(iter);
}

void CSocketManagerImpl::CloseSocket(const HudpHandle& handle) {
    // send close msg to remote here
    // TODO
}

std::shared_ptr<CSocket> CSocketManagerImpl::GetSocket(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter != _socket_map.end()) {
        return iter->second;
    }
    
    std::shared_ptr<CSocket> socket = std::make_shared<CSocket>(handle);
    _socket_map[handle] = socket;
    return socket;
}
