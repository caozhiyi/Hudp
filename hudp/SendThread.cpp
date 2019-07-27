#include "SendThread.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "OsNet.h"
#include "Log.h"

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
            COsNet::SendTo(_send_socket, msg->_bit_stream->GetDataPoint(), msg->_bit_stream->GetCurrentLength(), msg->_ip_port);

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