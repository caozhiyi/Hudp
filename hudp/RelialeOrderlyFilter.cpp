//#include "RelialeOrderlyFilter.h"
//#include "Log.h"
//#include "BitStreamPool.h"
//#include "NetMsgPool.h"
//#include "Serializes.h"
//#include "CommonType.h"
//#include "Socket.h"
//#include "SocketManager.h"
//#include "HudpImpl.h"
//#include "FunctionNetMsg.h"
//#include "CloseManager.h"
//
//using namespace hudp;
//
//bool CRelialeOrderlyFilter::OnSend(NetMsg* msg) {
//    auto share_pt = msg->_socket.lock();
//    if (!share_pt) {
//        base::LOG_ERROR("socket of msg is empty.");
//        return false;
//    }
//
//    if (msg->_head._flag & HPF_NEED_ACK && msg->_head._flag & HPF_IS_ORDERLY) {
//        share_pt->SendMsgToSendWnd(msg);
//       
//    } else if (msg->_head._flag & HPF_NEED_ACK) {
//        share_pt->SendMsgToSendWnd(msg);
//
//    } else {
//        // normal udp send to net direct
//        share_pt->SendMsgToNet(msg);
//    }
//    
//    return true;
//}
//
//bool CRelialeOrderlyFilter::OnRecv(NetMsg* msg) {
//    CBitStreamReader* temp_bit_stream = static_cast<CBitStreamReader*>(msg->_bit_stream);
//    if (!CSerializes::Deseriali(*temp_bit_stream, *msg)) {
//        base::LOG_ERROR("deserialize stream to msg failed. HudpHandle : %s", msg->_ip_port.c_str());
//        return false;
//    }
//
//    if (msg->_head._flag & HPF_CLOSE_ACK) {
//        CCloseManager::Instance().CloseAck(msg->_ip_port);
//        return true;
//    }
//
//    // close msg
//    if (msg->_head._flag & HPF_CLOSE) {
//        CCloseManager::Instance().RecvClose(msg->_ip_port);
//        return true;
//    }
//
//    std::shared_ptr<CSocket> socket;
//    CSocketManager::Instance().GetSocket(msg->_ip_port, socket);
//    msg->_socket = socket;
//
//    // with ack
//    if ((msg->_head._flag & HPF_WITH_RELIABLE_ACK || msg->_head._flag & HPF_WITH_RELIABLE_ORDERLY_ACK) && socket) {
//        socket->RecvAck(msg);
//    }
//
//    if (msg->_head._flag & HPF_NEED_ACK && msg->_head._flag & HPF_IS_ORDERLY) {
//        socket->RecvMsgToOrderList(msg);
//
//    } else if (msg->_head._flag & HPF_NEED_ACK) {
//        socket->RecvMsgToOrderList(msg);
//
//    } else if (msg->_head._body_len != 0){
//        CHudpImpl::Instance().SendMsgToUpper(msg);
//    }
//
//    return true;
//}