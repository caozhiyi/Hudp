#include "FunctionNetMsg.h"
#include "HudpImpl.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "FilterProcess.h"
#include "Log.h"
#include "Socket.h"

using namespace hudp;

void CSenderOrderlyNetMsg::ToSend() {
    // first into process filter. in process loop
    if (_phase == PP_HEAD_HANDLE) {
        NextPhase();

    } else {
        CFilterProcess::Instance().SendProcess(this);
    }
}

void CSenderOrderlyNetMsg::AckDone() {
    CBitStreamPool::Instance().FreeBitStream(this->_bit_stream);
    CNetMsgPool::Instance().FreeMsg(this);
}

void CSenderOrderlyNetMsg::Clear() {
    NetMsg::Clear();
    CSendWndSolt::Clear();
}

void CSenderRelialeOrderlyNetMsg::ToSend() {
    // first into process filter. in process loop
    if (_phase == PP_HEAD_HANDLE) {
        NextPhase();

    } else {
        CFilterProcess::Instance().SendProcess(this);
    }
    
}

void CSenderRelialeOrderlyNetMsg::AckDone() {
    CBitStreamPool::Instance().FreeBitStream(this->_bit_stream);
    CNetMsgPool::Instance().FreeMsg(this);
}

void CSenderRelialeOrderlyNetMsg::OnTimer() {
    if (_phase == PP_HEAD_HANDLE) {
        NextPhase();

    } else {
        // add to timer again
        auto socket = _socket.lock();
        if (socket) {
            // send to process again
            CFilterProcess::Instance().SendProcess(this);
            socket->AddToTimer(this);
            base::LOG_DEBUG("resend msg to net. id : %d", _head._id);

        } else {
            base::LOG_INFO("resend msg is destroyed");
        }
    }
}

void CSenderRelialeOrderlyNetMsg::Clear() {
    NetMsg::Clear();
    CSendWndSolt::Clear();
    CTimerSolt::Clear();
}

void CReceiverNetMsg::ToRecv() {
    CHudpImpl::Instance().SendMsgToUpper(this);

    // send ack msg to remote
    auto socket = _socket.lock();
    if (!socket) {
        base::LOG_ERROR("a recv net msg can't find socket");
        return;
    }

    socket->AddAck(this);
}

 void CReceiverNetMsg::Clear() {
     NetMsg::Clear();
 }