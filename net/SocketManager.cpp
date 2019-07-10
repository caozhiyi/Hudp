#include <cstring>		//for memset
#include "SocketManager.h"
#include "NetMsg.h"
#include "Socket.h"
#include "Hudp.h"

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

void CSocketManager::SendMsg(const HudpHandle& handle, NetMsg* msg) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto ptr = GetSocket(handle);
    msg->_socket = ptr;
    ptr->SendMsgToPriQueue(msg);
    _have_msg_socket.push_back(handle);
    _notify.notify_one();
}

void CSocketManager::RecvMsg(const HudpHandle& handle, NetMsg* msg) {
    // if a normal udp, send to upper direct.
    if (msg->_head._flag & HTF_NORMAL) {
        CHudp::Instance().SendMsgToUpper(msg);
    }
    
    // add to order list.
    std::unique_lock<std::mutex> lock(_mutex);
    auto ptr = GetSocket(handle);
    msg->_socket = ptr;
    ptr->RecvMsgToOrderList(msg);
}

void CSocketManager::Destroy(const HudpHandle& handle) {
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
    
    std::shared_ptr<CSocket> socket = std::make_shared<CSocket>(new CSocket());
    _socket_map[handle] = socket;
    return socket;
}