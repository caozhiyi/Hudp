#include "HudpImpl.h"
#include "SocketImpl.h"
#include "SocketManager.h"

using namespace hudp;

CSocketManagerImpl::CSocketManagerImpl() {

}

CSocketManagerImpl::~CSocketManagerImpl() {
    std::unique_lock<std::mutex> lock(_mutex);
    _socket_map.clear();
}

bool CSocketManagerImpl::IsSocketExist(const HudpHandle& handle) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return false;
    }
    return true;
}

std::shared_ptr<CSocket> CSocketManagerImpl::GetSocket(const HudpHandle& handle) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _socket_map.find(handle);
    if (iter != _socket_map.end()) {
        return iter->second;
    }

    std::shared_ptr<CSocket> socket = std::make_shared<CSocketImpl>(handle);
    _socket_map[handle] = socket;
    return socket;
}

bool CSocketManagerImpl::DeleteSocket(const HudpHandle& handle) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return false;
    }

    iter->second.reset();
    _socket_map.erase(iter);
    return true;
}

bool CSocketManagerImpl::CloseSocket(const HudpHandle& handle) {
    // send close msg to remote here
    std::shared_ptr<CSocket> sock;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto iter = _socket_map.find(handle);
        if (iter == _socket_map.end()) {
            return false;
        }
        sock = iter->second;
    }
    
    sock->SendFinMessage();
    return true;
}