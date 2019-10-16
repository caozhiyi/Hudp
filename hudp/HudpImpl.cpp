#include "HudpImpl.h"
#include "OsNet.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "NetMsg.h"
#include "Socket.h"
#include "FilterProcess.h"
#include "CloseManager.h"
#include "HudpConfig.h"

using namespace hudp;

CHudpImpl::CHudpImpl() {

}

CHudpImpl::~CHudpImpl() {
    _recv_process_thread.Stop();
    _send_process_thread.Stop();
    _recv_thread.Stop();
    _send_thread.Stop();
    _upper_thread.Stop();
    CTimer::Instance().Stop();
}

void CHudpImpl::Init() {
    static bool init_once = true; 
    if (init_once) {
        COsNet::Init();

        CFilterProcess::Instance().Init();

        if (__open_log) {
            base::CLog::Instance().SetLogName(__log_file_name);
            base::CLog::Instance().SetLogLevel(__log_level);
            base::CLog::Instance().Start();
        }
        
    }
}

bool CHudpImpl::Start(uint16_t port, const recv_back& func) {
    std::string ip = COsNet::GetOsIp();
    
    return Start(ip, port, func);
}

bool CHudpImpl::Start(const std::string& ip, uint16_t port, const recv_back& func) {
    uint64_t socket = COsNet::UdpSocket();
    if (socket == 0) {
        return false;
    }

    if (!COsNet::Bind(socket, ip, port)) {
        return false;
    }

    _recv_process_thread.Start();
    _send_process_thread.Start();
    _recv_thread.Start(socket);
    _send_thread.Start(socket);
    _upper_thread.Start(func);
    CTimer::Instance().Start();

    return true;
}

void CHudpImpl::Join() {
    _recv_process_thread.Join();
    _send_process_thread.Join();
    _recv_thread.Join();
    _send_thread.Join();
    _upper_thread.Join();
    CTimer::Instance().Join();

}

void CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg) {
    SendTo(handle, flag, msg.c_str(), msg.length());
}

void CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint16_t len) {
    if (len > __msg_body_size) {
        base::LOG_ERROR("msg size is bigger than msg bosy size.");
        return;
    }
    NetMsg* net_msg = CNetMsgPool::Instance().GetSendMsg(flag);
    net_msg->_head._flag = flag;
    net_msg->_head._body_len = len;
    net_msg->_phase = PP_HEAD_HANDLE;
    memcpy(net_msg->_body, msg, len);

    // not set priority, default HPF_LOW_PRI
    if (!(net_msg->_head._flag & (HPF_LOW_PRI | HPF_NROMAL_PRI | HPF_HIGH_PRI | HPF_HIGHEST_PRI))) {
        net_msg->_head._flag |= HPF_LOW_PRI;
    }
    TranslateFlag(net_msg);

    //get a socket. 
    std::shared_ptr<CSocket> socket;
    CSocketManager::Instance().GetSocket(handle, socket);
    net_msg->_socket = socket;
    net_msg->_ip_port = handle;

    // send msg to pri queue.
    socket->SendMsgToPriQueue(net_msg);
}

void CHudpImpl::Close(const HudpHandle& handle) {
    // send close msg to remote
    CCloseManager::Instance().StartClose(handle);
}

void CHudpImpl::SendMsgToNet(NetMsg* msg) {
    _send_thread.Push(msg);
}

void CHudpImpl::SendMsgToUpper(NetMsg* msg) {
    _upper_thread.Push(msg);
}

void CHudpImpl::SendMsgToRecvProcessThread(NetMsg* msg) {
    _recv_process_thread.Push(msg);
}

void CHudpImpl::SendMsgToSendProcessThread(NetMsg* msg) {
    _send_process_thread.Push(msg);
}

void CHudpImpl::TranslateFlag(NetMsg* msg) {
    if (msg->_head._flag & HTF_ORDERLY) {
        msg->_head._flag &= ~HTF_ORDERLY;
        msg->_head._flag |= HPF_IS_ORDERLY;

    } else if (msg->_head._flag & HTF_RELIABLE) {
        msg->_head._flag &= ~HTF_RELIABLE;
        msg->_head._flag |= HPF_NEED_ACK;

    } else if (msg->_head._flag & HTF_RELIABLE_ORDERLY) {
        msg->_head._flag &= ~HTF_RELIABLE_ORDERLY;
        msg->_head._flag |= HPF_NEED_ACK;
        msg->_head._flag |= HPF_IS_ORDERLY;
    
    } else if (msg->_head._flag & HTF_NORMAL) {
        msg->_head._flag &= ~HTF_NORMAL;
    
    }
}
