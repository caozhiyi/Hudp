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
        auto socket = _socket.lock();
        if (socket) {
            socket->AttachPendAck(this);
        }

    } else {
        auto socket = _socket.lock();
        if (socket) {
            socket->AttachPendAck(this);
        }
        CFilterProcess::Instance().SendProcess(this);
    }
    base::LOG_DEBUG("[sender] : send wnd send msg. id : %d", _head._id);
}

void CSenderOrderlyNetMsg::AckDone() {
    base::LOG_DEBUG("[ack] : send wnd ack done. id : %d", _head._id);
    CBitStreamPool::Instance().FreeBitStream(this->_bit_stream);
    CNetMsgPool::Instance().FreeMsg(this);
}

void CSenderOrderlyNetMsg::Clear() {
    base::LOG_DEBUG("[sender] : sender orderly msg clear. id : %d", _head._id);
    NetMsg::Clear();
    CSendWndSolt::Clear();
}

void CSenderRelialeOrderlyNetMsg::ToSend() {
    // first into process filter. in process loop
    if (_phase == PP_HEAD_HANDLE) {
        NextPhase();
        auto socket = _socket.lock();
        if (socket) {
            socket->AttachPendAck(this);
        }
    } else {
        auto socket = _socket.lock();
        if (socket) {
            socket->AttachPendAck(this);
        }
        CFilterProcess::Instance().SendProcess(this);
    }
    base::LOG_DEBUG("[sender] : send wnd send msg. id : %d", _head._id);
}

void CSenderRelialeOrderlyNetMsg::AckDone() {
    base::LOG_DEBUG("[ack] : send wnd ack done. id : %d", _head._id);
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
            ClearAck();
            socket->AttachPendAck(this);
            // send to process again
            CFilterProcess::Instance().SendProcess(this);
            base::LOG_DEBUG("[sender] : resend msg to net. id : %d", _head._id);
        }
    }
}

void CSenderRelialeOrderlyNetMsg::Clear() {
    base::LOG_DEBUG("[sender] : sender reliable orderly msg clear. id : %d", _head._id);
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

    base::LOG_DEBUG("[reveiver] :receiver msg. id : %d", _head._id);
    socket->AddAck(this);
}

 void CReceiverNetMsg::Clear() {
     base::LOG_DEBUG("[reveiver] : reveiver msg clear. id : %d", _head._id);
     NetMsg::Clear();
 }