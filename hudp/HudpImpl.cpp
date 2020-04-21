#include <functional>

#include "Log.h"
#include "IMsg.h"
#include "OsNet.h"
#include "Timer.h"
#include "ISocket.h"
#include "HudpImpl.h"
#include "EndFilter.h"
#include "HeadFilter.h"
#include "HudpConfig.h"
#include "RecvThread.h"
#include "SnappyFilter.h"
#include "PriorityQueue.h"
#include "SocketManager.h"
#include "ProcessThread.h"
#include "MsgPoolFactory.h"
#include "UpperThreadFilter.h"
#include "FlowSlicingFilter.h"
#include "FilterProcessNoThread.h"
#include "FilterProcessWithThread.h"

using namespace hudp;

CHudpImpl::CHudpImpl() {
    _net_io          = std::make_shared<COsNetImpl>();
    _recv_thread     = std::make_shared<CRecvThread>();
    _process_thread  = std::make_shared<CProcessThread>();
    _socket_mananger = std::make_shared<CSocketManagerImpl>();
    if (__filter_whit_thread) {
        _filter_process  = std::make_shared<CFilterProcessWithThread>();
    } else {
        _filter_process  = std::make_shared<CFilterProcessNoThread>();
    }
}

CHudpImpl::~CHudpImpl() {
    CTimer::Instance().Stop();
    _net_io->Close(_listen_socket);
}

hudp_error_code CHudpImpl::Init() {
    static bool init_once = true; 
    if (init_once) {
        _net_io->Init();

        if (__open_log) {
            base::CLog::Instance().SetLogName(__log_file_name);
            base::CLog::Instance().SetLogLevel(__log_level);
            base::CLog::Instance().Start();
        }
    }
    /*******************add body filter here.********************/
    if (__upper_with_thread) {
        _filter_process->AddFilter(std::shared_ptr<CFilter>(new CUpperThreadFilter()));
    }
    _filter_process->AddFilter(std::shared_ptr<CFilter>(new CHeadFilter()));
    _filter_process->AddFilter(std::shared_ptr<CFilter>(new CSnappyFilter()));
    _filter_process->AddFilter(std::shared_ptr<CFilter>(new CFlowSlicingFilter()));
    _filter_process->AddFilter(std::shared_ptr<CFilter>(new CEndFilter()));
    /*******************add body filter here.********************/

    return HEC_SUCCESS;
}

hudp_error_code CHudpImpl::Start(const std::string& ip, uint16_t port, const recv_back& recv_func, 
                                 const can_write_back& can_write_func) {
    if (!recv_func || !can_write_func) {
        base::LOG_ERROR("call back is null");
        return HEC_INVALID_PARAM;
    }
    _recv_call_back      = recv_func;
    _can_write_call_back = can_write_func;
    // create udp socket
    _listen_socket = _net_io->UdpSocket();
    if (_listen_socket == 0) {
        return HEC_FAILED;
    }

    if (!_net_io->Bind(_listen_socket, ip, port)) {
        return HEC_FAILED;
    }

    _process_thread->Start(_filter_process);
    _recv_thread->Start(_listen_socket, _net_io);
    CTimer::Instance().Start();

    return HEC_SUCCESS;
}

hudp_error_code CHudpImpl::Join() {
    _process_thread->Join();
    _recv_thread->Join();
    CTimer::Instance().Join();

    return HEC_SUCCESS;
}

hudp_error_code CHudpImpl::SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg) {
    if (msg.length() > __msg_body_size) {
        base::LOG_ERROR("msg size is bigger than msg body size.");
        return HEC_INVALID_PARAM;
    }

    //get a socket. 
    std::shared_ptr<CSocket> sock = _socket_mananger->GetSocket(handle);
    if (!sock->CanSendMessage()) {
        base::LOG_ERROR("the socket is not ready.");
        return HEC_FAILED;
    }

    std::shared_ptr<CMsg> net_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    net_msg->SetFlag(msg_send);
    net_msg->SetHeaderFlag(flag);
    net_msg->SetHandle(handle);
    net_msg->SetBody(msg);
    net_msg->SetSocket(sock);

    _process_thread->Push(net_msg);
    return HEC_SUCCESS;
}

hudp_error_code CHudpImpl::CheckCanSend(const HudpHandle& handle) {
    //get a socket. 
    std::shared_ptr<CSocket> sock = _socket_mananger->GetSocket(handle);
    if (!sock->CanSendMessage()) {
        base::LOG_ERROR("the socket is not ready.");
        return HEC_FAILED;
    }
    return HEC_SUCCESS;
}

hudp_error_code CHudpImpl::ConnectTo(const std::string& ip, uint16_t port) {
    return HEC_SUCCESS;
}

hudp_error_code CHudpImpl::Close(const HudpHandle& handle) {
    // send close msg to remote
    if (_socket_mananger->CloseSocket(handle)) {
        return HEC_SUCCESS;
    }
    return HEC_FAILED;
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

void CHudpImpl::RecvMessageToUpper(const HudpHandle& handle, std::string& body) {
    _recv_call_back(handle, body.c_str(), (uint16_t)body.length());
}

void CHudpImpl::SendMessageToNet(std::shared_ptr<CMsg> msg) {
    // get send buffer
    std::string net_msg = msg->GetSerializeBuffer();
    _net_io->SendTo(_listen_socket, net_msg.c_str(), (uint32_t)net_msg.length(), msg->GetHandle());
}

CPriorityQueue* CHudpImpl::CreatePriorityQueue() {
    return new CPriorityQueueImpl();
}

void CHudpImpl::ReleaseSocket(const HudpHandle& handle) {
    _socket_mananger->DeleteSocket(handle);
}

bool CHudpImpl::SendMsgToFilter(const HudpHandle& handle, uint16_t flag, std::string& msg) {
    return _filter_process->PushSendMsg(handle, flag, msg);
}

bool CHudpImpl::RecvMsgToFilter(const HudpHandle& handle, uint16_t flag, std::string& msg) {
    return _filter_process->PushRecvMsg(handle, flag, msg);
}