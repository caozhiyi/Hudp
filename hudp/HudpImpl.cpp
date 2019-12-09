#include <functional>
#include "HudpImpl.h"
#include "HudpConfig.h"
#include "IMSg.h"
#include "Timer.h"
#include "Log.h"
#include "RecvThread.h"
#include "ProcessThread.h"
#include "OsNet.h"
#include "MsgPoolFactory.h"
#include "FilterProcessNoThread.h"
#include "SocketManager.h"
#include "ISocket.h"

using namespace hudp;

CHudpImpl::CHudpImpl() {
    _net_io = std::make_shared<COsNetImpl>();
    _msg_factory = std::make_shared<CMsgPoolFactory>();
    _filter_process = std::make_shared<CFilterProcessNoThread>();
    _socket_mananger = std::make_shared<CSocketManagerImpl>();
    _process_thread = std::make_shared<CProcessThread>();
    _recv_thread = std::make_shared<CRecvThread>();
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
        _filter_process->SetSendFunc(std::bind(&CHudpImpl::AfterSendProcess, this, std::placeholders::_1));
        _filter_process->SetRecvFunc(std::bind(&CHudpImpl::AfterRecvProcess, this, std::placeholders::_1));
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

void CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg) {
    SendTo(handle, flag, msg.c_str(), msg.length());
}

void CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint16_t len) {
    if (len > __msg_body_size) {
        base::LOG_ERROR("msg size is bigger than msg body size.");
        return;
    }

    CMsg* net_msg = _msg_factory->CreateMsg();
    net_msg->SetFlag(msg_send);
    net_msg->SetHeaderFlag(flag);
    net_msg->SetHandle(handle);
    net_msg->SetBody(std::string(msg, len));

    // push msg to process thread
    _process_thread->Push(net_msg);
}

void CHudpImpl::RecvMsg(const HudpHandle& handle, const std::string& msg) {
    CMsg* net_msg = _msg_factory->CreateMsg();
    if (!net_msg->InitWithBuffer(msg)) {
        base::LOG_ERROR("parser msg error.");
        _msg_factory->DeleteMsg(net_msg);
    }
    net_msg->SetFlag(msg_recv);
    // push msg to process thread
    _process_thread->Push(net_msg);
}

void CHudpImpl::Close(const HudpHandle& handle) {
    // send close msg to remote
    _socket_mananger->CloseSocket(handle);
}

void CHudpImpl::RecvMessageToUpper(const HudpHandle& HudpHandle, CMsg* msg) {
    _filter_process->PushRecvMsg(msg);
}

void CHudpImpl::SendMessageToNet(CMsg* msg) {
    // get send buffer
    msg->TranslateFlag();
    std::string net_msg = msg->GetSerializeBuffer();
    _net_io->SendTo(_listen_socket, net_msg.c_str(), net_msg.length(), msg->GetHandle());
}

void CHudpImpl::ReleaseMessage(CMsg* msg) {
    _msg_factory->DeleteMsg(msg);
}

CMsg* CHudpImpl::CreateMessage() {
    return _msg_factory->CreateMsg();
}

void CHudpImpl::AfterSendProcess(CMsg* msg) {
    //get a socket. 
    std::shared_ptr<CSocket> sock = _socket_mananger->GetSocket(msg->GetHandle());

    sock->SendMessage(msg);
}

void CHudpImpl::AfterRecvProcess(CMsg* msg) {
    std::string& body = msg->GetBody();
    _recv_call_back(msg->GetHandle(), body.c_str(), body.length());
    _msg_factory->DeleteMsg(msg);
}