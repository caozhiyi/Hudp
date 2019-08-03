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
    auto socket = _socket.lock();
    if (socket) {
        // send to process again
        socket->SendMsgToNet(this);
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
    auto socket = _socket.lock();
    if (socket) {
        // send to process again
        socket->SendMsgToNet(this);
    }
    base::LOG_DEBUG("[sender] : send wnd send msg. id : %d", _head._id);
}

void CSenderRelialeOrderlyNetMsg::AckDone() {
    base::LOG_DEBUG("[ack] : send wnd ack done. id : %d", _head._id);
    CBitStreamPool::Instance().FreeBitStream(this->_bit_stream);
    CNetMsgPool::Instance().FreeMsg(this);
}

void CSenderRelialeOrderlyNetMsg::OnTimer() {
    // add to timer again
    auto socket = _socket.lock();
    if (socket) {
        // send to process again
        socket->SendMsgToNet(this);
        base::LOG_DEBUG("[sender] : resend msg to net. id : %d", _head._id);
    }
}

void CSenderRelialeOrderlyNetMsg::Clear() {
    base::LOG_DEBUG("[sender] : sender reliable orderly msg clear. id : %d", _head._id);
    // first remove from timer
    CTimerSolt::Clear();
    NetMsg::Clear();
    CSendWndSolt::Clear();
}

void CReceiverNetMsg::ToRecv() {
    // send ack msg to remote
    auto socket = _socket.lock();
    if (!socket) {
        base::LOG_ERROR("[reveiver] a recv net msg can't find socket");
        return;
    }

    base::LOG_DEBUG("[reveiver] :receiver msg. id : %d", _head._id);
    socket->AddAck(this);

    CHudpImpl::Instance().SendMsgToUpper(this);
}

 void CReceiverNetMsg::Clear() {
     base::LOG_DEBUG("[reveiver] : reveiver msg clear. id : %d", _head._id);
     NetMsg::Clear();
 }