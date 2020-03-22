#include <functional>

#include "Log.h"
#include "IMsg.h"
#include "OsNet.h"
#include "Timer.h"
#include "ISocket.h"
#include "HudpImpl.h"
#include "HudpConfig.h"
#include "RecvThread.h"
#include "PriorityQueue.h"
#include "SocketManager.h"
#include "ProcessThread.h"
#include "MsgPoolFactory.h"
#include "FilterProcessNoThread.h"

using namespace hudp;

CHudpImpl::CHudpImpl() {
    _net_io          = std::make_shared<COsNetImpl>();
    _recv_thread     = std::make_shared<CRecvThread>();
    _process_thread  = std::make_shared<CProcessThread>();
    _socket_mananger = std::make_shared<CSocketManagerImpl>();
    _filter_process  = std::make_shared<CFilterProcessNoThread>();
}

CHudpImpl::~CHudpImpl() {
    CTimer::Instance().Stop();
    _net_io->Close(_listen_socket);
}

void CHudpImpl::Init() {
    static bool init_once = true; 
    if (init_once) {
        _net_io->Init();

        if (__open_log) {
            base::CLog::Instance().SetLogName(__log_file_name);
            base::CLog::Instance().SetLogLevel(__log_level);
            base::CLog::Instance().Start();
        }
        _filter_process->SetSendFunc(std::bind(&CHudpImpl::AfterSendFilter, this, std::placeholders::_1));
        _filter_process->SetRecvFunc(std::bind(&CHudpImpl::AfterRecvFilter, this, std::placeholders::_1));
    }
}

bool CHudpImpl::Start(const std::string& ip, uint16_t port, const recv_back& func) {
    if (!func) {
        base::LOG_ERROR("recv call back is null");
        return false;
    }
    _recv_call_back = func;

    // create udp socket
    _listen_socket = _net_io->UdpSocket();
    if (_listen_socket == 0) {
        return false;
    }

    if (!_net_io->Bind(_listen_socket, ip, port)) {
        return false;
    }

    _process_thread->Start(_filter_process);
    _recv_thread->Start(_listen_socket, _net_io);
    CTimer::Instance().Start();

    return true;
}

void CHudpImpl::Join() {
    _process_thread->Join();
    _recv_thread->Join();
    CTimer::Instance().Join();
}

bool CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg) {
    return SendTo(handle, flag, msg.c_str(), msg.length());
}

bool CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint16_t len) {
    if (len > __msg_body_size) {
        base::LOG_ERROR("msg size is bigger than msg body size.");
        return false;
    }

    //get a socket. 
    std::shared_ptr<CSocket> sock = _socket_mananger->GetSocket(handle);
    if (!sock->CanSendMessage()) {
        base::LOG_ERROR("the socket is not ready.");
        return false;
    }

    std::shared_ptr<CMsg> net_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    net_msg->SetFlag(msg_send);
    net_msg->SetHeaderFlag(flag);
    net_msg->SetHandle(handle);
    net_msg->SetBody(std::string(msg, len));
    net_msg->SetSocket(sock);

    // push msg to process thread
    _process_thread->Push(net_msg);
    return true;
}

void CHudpImpl::RecvMsgFromNet(const HudpHandle& handle, const std::string& msg) {
    std::shared_ptr<CMsg> net_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    if (!net_msg->InitWithBuffer(msg)) {
        base::LOG_ERROR("parser msg error.");
    }
    std::string flag_str = net_msg->DebugHeaderFlag();
    base::LOG_DEBUG("get message flag:%s", flag_str.c_str());
    net_msg->SetFlag(msg_recv);
    net_msg->SetHandle(handle);
    //get a socket. 
    std::shared_ptr<CSocket> sock = _socket_mananger->GetSocket(handle);
    net_msg->SetSocket(sock);
    // push msg to process thread
    _process_thread->Push(net_msg);
}

void CHudpImpl::Close(const HudpHandle& handle) {
    // send close msg to remote
    _socket_mananger->CloseSocket(handle);
}

void CHudpImpl::RecvMessageToUpper(const HudpHandle& HudpHandle, std::shared_ptr<CMsg> msg) {
    _filter_process->PushRecvMsg(msg);
}

void CHudpImpl::SendMessageToNet(std::shared_ptr<CMsg> msg) {
    // get send buffer
    std::string net_msg = msg->GetSerializeBuffer();
    _net_io->SendTo(_listen_socket, net_msg.c_str(), net_msg.length(), msg->GetHandle());
}

CPriorityQueue* CHudpImpl::CreatePriorityQueue() {
    return new CPriorityQueueImpl();
}

void CHudpImpl::ReleaseSocket(const HudpHandle& handle) {
    _socket_mananger->DeleteSocket(handle);
}

void CHudpImpl::AfterSendFilter(std::shared_ptr<CMsg> msg) {
    auto sock = msg->GetSocket();
    sock->SendMessage(msg);
}

void CHudpImpl::AfterRecvFilter(std::shared_ptr<CMsg> msg) {
    std::string& body = msg->GetBody();
    _recv_call_back(msg->GetHandle(), body.c_str(), body.length());
}