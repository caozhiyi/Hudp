#include "FunctionNetMsg.h"
#include "Hudp.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"

using namespace hudp;

void CSenderOrderlyNetMsg::ToSend() {
    CHudp::Instance().SendMsgToNet(this);
}

void CSenderOrderlyNetMsg::AckDone() {
    CBitStreamPool::Instance().FreeBitStream(this->_bit_stream);
    CNetMsgPool::Instance().FreeMsg(this);
}

void CSenderRelialeOrderlyNetMsg::ToSend() {
    CHudp::Instance().SendMsgToNet(this);
}

void CSenderRelialeOrderlyNetMsg::AckDone() {
    CBitStreamPool::Instance().FreeBitStream(this->_bit_stream);
    CNetMsgPool::Instance().FreeMsg(this);
}

void CSenderRelialeOrderlyNetMsg::OnTimer() {
    CHudp::Instance().SendMsgToNet(this);
}

void CReceiverNetMsg::ToRecv() {
    CHudp::Instance().SendMsgToUpper(this);

    // send ack msg to remote
    
}
