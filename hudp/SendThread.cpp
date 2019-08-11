#include "SendThread.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "OsNet.h"
#include "Log.h"

#ifdef NET_LOSS_TEST
#include <random>
static const int __packet_loss = 2;
#endif

using namespace hudp;

CSendThread::CSendThread() : _send_socket(0) {

}

CSendThread::~CSendThread() {
    Stop();
    Push(nullptr);
    Join();
}

void CSendThread::Start(uint64_t socket) {
    _send_socket = socket;
    CRunnable::Start();
}

void CSendThread::Run() {
    if (_send_socket == 0) {
        base::LOG_ERROR("send thread should set send socket in Start().");
        return;
    }

    while (!_stop) {
        auto msg = static_cast<NetMsg*>(_Pop());
        // msg not free to pool
        if (msg && !msg->_ip_port.empty()) {
#ifdef NET_LOSS_TEST
            if (GetRandomNumber() > __packet_loss) {
                COsNet::SendTo(_send_socket, msg->_bit_stream->GetDataPoint(), msg->_bit_stream->GetCurrentLength(), msg->_ip_port);

            } else {
                base::LOG_DEBUG("a msg loss. id : %d", msg->_head._id);
            }
#else
            COsNet::SendTo(_send_socket, msg->_bit_stream->GetDataPoint(), msg->_bit_stream->GetCurrentLength(), msg->_ip_port);
#endif

            // if msg don't need ack, destroy here
            if (!(msg->_head._flag & HPF_NEED_ACK) && msg->_use) {
                CBitStreamPool::Instance().FreeBitStream(msg->_bit_stream);
                CNetMsgPool::Instance().FreeMsg(msg);
            }

        } else {
            base::LOG_WARN("send thread get a null msg.");
        }
    }
}

#ifdef NET_LOSS_TEST
int CSendThread::GetRandomNumber() {
    static std::default_random_engine engine;
    std::uniform_int_distribution<unsigned> u(0, 9);
    int ret = u(engine);
    return ret;
}
#endif