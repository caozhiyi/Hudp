#include "RelialeOrderlyFilter.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "Serializes.h"
#include "CommonType.h"
#include "Socket.h"
#include "SocketManager.h"

using namespace hudp;

bool CRelialeOrderlyFilter::OnSend(NetMsg* msg) {
    auto share_pt = msg->_socket.lock();
    if (!share_pt) {
        base::LOG_ERROR("socket of msg is empty.");
        return false;
    }

    // normal udp send to net direct
    if (msg->_head._flag & HTF_NORMAL) {
        share_pt->SendMsgToNet(msg);

    } else {
        share_pt->SendMsgToSendWnd(msg);
    }
    
    return true;
}

bool CRelialeOrderlyFilter::OnRecv(NetMsg* msg) {
    //get a socket for msg
    std::shared_ptr<CSocket> socket;
    if (CSocketManager::Instance().GetRecvSocket(msg->_ip_port, msg->_head._flag, socket)) {
        socket->RecvMsgToOrderList(msg);
    
    } else {
        socket->RecvMsgUpper(msg);
    }
    
    return true;
}