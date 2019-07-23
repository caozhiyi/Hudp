#include "RelialeOrderlyFilter.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "Serializes.h"
#include "CommonType.h"
#include "Socket.h"
#include "SocketManager.h"
#include "HudpImpl.h"
#include "FunctionNetMsg.h"

using namespace hudp;

bool CRelialeOrderlyFilter::OnSend(NetMsg* msg) {
    auto share_pt = msg->_socket.lock();
    if (!share_pt) {
        base::LOG_ERROR("socket of msg is empty.");
        return false;
    }

    // normal udp send to net direct
    if (!(msg->_head._flag & HPF_NEED_ACK && msg->_head._flag & HPF_IS_ORDERLY)) {
        // set msg to next phase 
        static_cast<CSenderOrderlyNetMsg*>(msg)->NextPhase();

    } else {
        share_pt->SendMsgToSendWnd(msg);
    }
    
    return true;
}

bool CRelialeOrderlyFilter::OnRecv(NetMsg* msg) {

    std::shared_ptr<CSocket> socket;
    CSocketManager::Instance().GetRecvSocket(msg->_ip_port, msg->_head._flag, socket);
    
    // with ack
    if ((msg->_head._flag & HPF_WITH_ACK_ARRAY || msg->_head._flag & HPF_WITH_ACK_RANGE) && socket) {
        socket->RecvAck(msg);
    }

    if (!(msg->_head._flag & HPF_NEED_ACK && msg->_head._flag & HPF_IS_ORDERLY) && msg->_head._body_len != 0) {
        // set msg to next phase 
        // static_cast<CReceiverNetMsg*>(msg)->NextPhase();

        // now don't have other phase, send to upper
        CHudpImpl::Instance().SendMsgToUpper(msg);

    } else if (socket && msg->_head._body_len != 0){
        msg->_socket = socket;
        socket->RecvMsgToOrderList(msg);

    } else if (!socket) {
        base::LOG_ERROR("get a recv socket error.");
        return false;
    }

    return true;
}