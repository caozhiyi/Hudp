#include "Hudp.h"
#include "OsNet.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "NetMsg.h"
#include "Socket.h"

using namespace hudp;

bool CHudp::Start(uint16_t port, const recv_back& func) {
    uint64_t socket = COsNet::UdpSocket();
    if (socket == 0) {
        return false;
    }

    std::string ip = COsNet::GetOsIp();
    if (!COsNet::Bind(socket, ip, port)) {
        return false;
    }

    _recv_process_thread.Start();
    _send_process_thread.Start();
    _recv_thread.Start(socket);
    _send_thread.Start(socket);
    _upper_thread.Start(func);

    return true;
}

void CHudp::SendTo(const HudpHandle& handlle, uint16_t flag, const std::string& msg) {
    SendTo(handlle, flag, msg.c_str(), msg.length());
}

void CHudp::SendTo(const HudpHandle& handlle, uint16_t flag, const char* msg, uint16_t len) {
    if (len > __body_size) {
        base::LOG_ERROR("msg size is bigger than msg bosy size.");
        return;
    }
    NetMsg* net_msg = CNetMsgPool::Instance().GetSendMsg(flag);
    net_msg->_head._flag = flag;
    net_msg->_head._body_len = len;
    memcpy(net_msg->_body, msg, len);
    
    //get a socket. 
    std::shared_ptr<CSocket> socket;
    CSocketManager::Instance().GetSendSocket(handlle, socket);
    net_msg->_socket = socket;

    // send msg to pri queue.
    socket->SendMsgToPriQueue(net_msg);
}

void CHudp::Destroy(const HudpHandle& handlle) {
    CSocketManager::Instance().Destroy(handlle);
}

void CHudp::SendMsgToNet(NetMsg* msg) {
    _send_thread.Push(msg);
}

void CHudp::SendMsgToUpper(NetMsg* msg) {
    _upper_thread.Push(msg);
}

void CHudp::SendMsgToRecvProcessThread(NetMsg* msg) {
    _recv_process_thread.Push(msg);
}

NetMsg* CHudp::GetMsgFromPriQueue() {
   return CSocketManager::Instance().GetMsg();
}