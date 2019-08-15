#include "CloseManager.h"
#include "NetMsg.h"
#include "Socket.h"
#include "HudpImpl.h"
#include "NetMsgPool.h"
#include "HudpImpl.h"
#include "SocketManager.h"

using namespace hudp;

CCloseManager::CCloseManager() {

}

CCloseManager::~CCloseManager() {

}

void CCloseManager::StartClose(const HudpHandle& handle) {
    // don't have this socket
    if (!CSocketManager::Instance().Exist(handle)) {
        return;
    }

    // get socket
    std::shared_ptr<CSocket> socket;
    CSocketManager::Instance().GetSocket(handle, socket);
    
    // send a close msg
    NetMsg* msg = CNetMsgPool::Instance().GetSendMsg(HTF_RELIABLE);
    msg->_head._flag |= HPF_CLOSE | HPF_NEED_ACK;
    msg->_socket = socket;
    msg->_ip_port = socket->GetHandle();
    CHudpImpl::Instance().SendMsgToSendProcessThread(msg);
}

void CCloseManager::RecvClose(const HudpHandle& handle) {
    // we need send a ack
    NetMsg* msg = CNetMsgPool::Instance().GetNormalMsg();
    msg->_head._flag |= HPF_CLOSE_ACK;
    msg->_ip_port = handle;
    CHudpImpl::Instance().SendMsgToSendProcessThread(msg);

    // don't have this socket. 
    if (!CSocketManager::Instance().Exist(handle)) {
        return;
    }

    // start to close
    StartClose(handle);
}

bool CCloseManager::CloseAck(const HudpHandle& handle) {
    // destory socket
    CSocketManager::Instance().Destroy(handle);
}