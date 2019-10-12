#include "SocketManager.h"
#include "NetMsg.h"
#include "Socket.h"
#include "HudpImpl.h"

using namespace hudp;

CSocketManager::CSocketManager() {

}

CSocketManager::~CSocketManager() {
    _socket_map.clear();
}

NetMsg* CSocketManager::GetMsg() {
    std::unique_lock<std::mutex> lock(_mutex);
    _notify.wait(_mutex, [this]() {return this->_have_msg_socket.size() != 0; });

    auto iter = _have_msg_socket.begin();
    auto msg = _socket_map[*iter]->GetMsgFromPriQueue();
    _have_msg_socket.erase(iter);
    return msg;
}

void CSocketManager::NotifyMsg(const HudpHandle& handle) {
    std::unique_lock<std::mutex> lock(_mutex);
    _have_msg_socket.push_back(handle);
    _notify.notify_one();
}

void CSocketManager::GetSocket(const HudpHandle& handle, std::shared_ptr<CSocket>& socket) {
    std::unique_lock<std::mutex> lock(_mutex);
    socket = GetSocket(handle);
}

void CSocketManager::Destroy(const HudpHandle& handle) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return;
    }

    iter->second.reset();
    _socket_map.erase(iter);
}

 bool CSocketManager::Exist(const HudpHandle& handle) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return false;
    }
    return true;
 }

std::shared_ptr<CSocket> CSocketManager::GetSocket(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter != _socket_map.end()) {
        return iter->second;
    }
    
    std::shared_ptr<CSocket> socket = std::make_shared<CSocket>(handle);
    _socket_map[handle] = socket;
    return socket;
}
