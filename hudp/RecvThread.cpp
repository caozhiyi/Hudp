#include "RecvThread.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "FunctionNetMsg.h"
#include "OsNet.h"
#include "Log.h"
#include "Hudp.h"

using namespace hudp;

static const uint16_t __recv_buf_size = 2048;

CRecvThread::CRecvThread() : _recv_socket(0) {

}

CRecvThread::~CRecvThread() {
    Stop();
    COsNet::SendTo(_recv_socket, "1", 1);
    Join();
    COsNet::Close(_recv_socket);
}

void CRecvThread::Start(uint64_t socket) {
    _recv_socket = socket;
    CRunnable::Start();
}

void CRecvThread::Run() {
    if (_recv_socket == 0) {
        base::LOG_ERROR("recv thread should set recv socket.");
        return;
    }

    char buf[__recv_buf_size] = {0};
    std::string ip;
    uint16_t port = 0;
    int ret = 0;
    while (!_stop) {
        memset(buf, 0, ret);
        ret = 0;
        // recv msg from net
        ret = COsNet::RecvFrom(_recv_socket, buf, __recv_buf_size, ip, port);
        if (ret > 0) {
            CBitStreamReader* bit_stream = static_cast<CBitStreamReader*>(CBitStreamPool::Instance().GetBitStream());
            bit_stream->Init(buf, ret);

            CReceiverNetMsg* msg = static_cast<CReceiverNetMsg*>(CNetMsgPool::Instance().GetRecvMsg());
            msg->_ip_port = ip + std::to_string(port);
            msg->_bit_stream = bit_stream;
            
            CHudp::Instance().SendMsgToRecvProcessThread(msg);
        }
    }
}