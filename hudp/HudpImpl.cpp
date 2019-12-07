#include "HudpImpl.h"
#include "INetIO.h"
#include "IThread.h"
#include "IMsgFactory.h"
#include "HudpConfig.h"
#include "ISocketManager.h"
#include "ISocket.h"
#include "IMSg.h"
#include "IFilterProcess.h"
#include "Timer.h"
#include "Log.h"

using namespace hudp;

CHudpImpl::CHudpImpl() {

}

CHudpImpl::~CHudpImpl() {
    for (auto iter = _thread_vec.begin(); iter != _thread_vec.end(); ++iter) {
        (*iter)->Stop();
    }
    //CTimer::Instance().Stop();
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

    for (auto iter = _thread_vec.begin(); iter != _thread_vec.end(); ++iter) {
        (*iter)->Start();
    }
    
    //CTimer::Instance().Start();

    return true;
}

void CHudpImpl::Join() {
    for (auto iter = _thread_vec.begin(); iter != _thread_vec.end(); ++iter) {
        (*iter)->Join();
    }

    //CTimer::Instance().Join();
}

void CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg) {
    SendTo(handle, flag, msg.c_str(), msg.length());
}

void CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint16_t len) {
    if (len > __msg_body_size) {
        base::LOG_ERROR("msg size is bigger than msg body size.");
        return;
    }

    CMsg* net_msg = _msg_factory->CreateMsg(flag);
    net_msg->TranslateFlag();
    net_msg->SetHandle(handle);
    net_msg->SetBody(std::string(msg, len));

    _filter_process->PushSendMsg(net_msg);
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
    std::string net_msg = msg->GetSerializeBuffer();
    _net_io->SendTo(_listen_socket, net_msg.c_str(), net_msg.length(), msg->GetHandle());
}

void CHudpImpl::ReleaseMessage(CMsg* msg) {
    _msg_factory->DeleteMsg(msg);
}

void CHudpImpl::AfterSendProcess(CMsg* msg) {
    //get a socket. 
    std::shared_ptr<CSocket> socket = _socket_mananger->GetSocket(msg->GetHandle());

    socket->SendMessage(msg);
}

void CHudpImpl::AfterRecvProcess(CMsg* msg) {
    std::string& body = msg->GetBody();
    _recv_call_back(msg->GetHandle(), body.c_str(), body.length());
    _msg_factory->DeleteMsg(msg);
}