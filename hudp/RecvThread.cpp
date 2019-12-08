#include "RecvThread.h"
#include "Log.h"
#include "HudpImpl.h"
#include "HudpConfig.h"
#include "INetIO.h"
#include "IMsgFactory.h"

using namespace hudp;

CRecvThread::CRecvThread() : _recv_socket(0), 
                             _msg_factory(nullptr), 
                             _net_io(nullptr) {

}

CRecvThread::~CRecvThread() {
    Stop();
    _net_io->SendTo(_recv_socket, "1", 1);
    Join();
    _net_io->Close(_recv_socket);
}

void CRecvThread::Start(uint64_t sock, std::shared_ptr<CMsgFactory>& msg_factory, std::shared_ptr<CNetIO>& net_io) {
    _recv_socket = sock;
    _msg_factory = msg_factory;
    _net_io = net_io;
    CRunnable::Start();
}

void CRecvThread::Run() {
    if (_recv_socket == 0) {
        base::LOG_ERROR("recv thread should set recv socket.");
        return;
    }

    char buf[__recv_buf_size];
    std::string ip;
    uint16_t port = 0;
    int ret = 0;
    while (!_stop) {
        ret = 0;
        // recv msg from net
        ret = _net_io->RecvFrom(_recv_socket, buf, __recv_buf_size, ip, port);
        if (ret > 0) {
            
            CHudpImpl::Instance().RecvMsg(ip + std::to_string(port), std::string(buf, ret));
        }
    }
}
