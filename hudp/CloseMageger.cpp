#include "CloseManager.h"
#include "NetMsg.h"
#include "Socket.h"
#include "HudpImpl.h"
#include "NetMsgPool.h"
#include "HudpImpl.h"
#include "SocketManager.h"
#include "BitStreamPool.h"
#include "Serializes.h"
#include "Log.h"

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
    msg->_head._flag |= HPF_CLOSE | HPF_NEED_ACK | HPF_NROMAL_PRI;
    msg->_socket = socket;
    msg->_ip_port = socket->GetHandle();
    msg->_phase = PP_HEAD_HANDLE;
    socket->SendMsgToPriQueue(msg);

    base::LOG_DEBUG("close socket . handle : %s", msg->_ip_port.c_str());
}

void CCloseManager::RecvClose(const HudpHandle& handle) {
    // we need send a ack
    NetMsg* msg = CNetMsgPool::Instance().GetNormalMsg();
    msg->_head._flag |= HPF_CLOSE_ACK;
    msg->_ip_port = handle;

    CBitStreamWriter* temp_bit_stream = static_cast<CBitStreamWriter*>(CBitStreamPool::Instance().GetBitStream());

    if (CSerializes::Serializes(*msg, *temp_bit_stream)) {
        msg->_bit_stream = temp_bit_stream;

    } else {
        base::LOG_ERROR("serializes msg to stream failed. handle : %s", msg->_ip_port.c_str());
        return;
    }

    CHudpImpl::Instance().SendMsgToNet(msg);
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
    return true;
}
