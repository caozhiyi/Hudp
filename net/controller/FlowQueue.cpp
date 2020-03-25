#include "IMsg.h"
#include "FlowQueue.h"

using namespace hudp;

void CFlowQueue::Add(std::shared_ptr<CMsg> msg) {
    // if msg is resend
    if (msg->GetFlag() & msg_resend) {
        AddToResendTail(msg);

    } else {
        AddToNormalTail(msg);
    }
}

std::shared_ptr<CMsg> CFlowQueue::Get() {
    std::shared_ptr<CMsg> ret = nullptr;
    if (_resend_head) {
        ret = _resend_head;
        _resend_head = _resend_head->GetNext();
        Remove(ret);
    }
    if (_normal_head) {
        ret = _normal_head;
        _normal_head = _normal_head->GetNext();
        Remove(ret);
    }
    return ret;
}

void CFlowQueue::Remove(std::shared_ptr<CMsg> msg) {
    if (!msg) {
        return;
    }
    if (msg == _normal_head) {
        _normal_head = _normal_head->GetNext();
    }
    if (msg == _normal_end) {
        _normal_end = _normal_end->GetPrev();
    }
    if (msg == _normal_head) {
        _normal_head = _normal_head->GetPrev();
    }
    if (msg == _resend_end) {
        _resend_end = _resend_end->GetPrev();
    }
    
    if (msg->GetPrev()) {
        msg->GetPrev()->SetNext(msg->GetNext());
    }
    if (msg->GetNext()) {
        msg->GetNext()->SetPrev(msg->GetPrev());
    }
}

void CFlowQueue::AddToNormalHead(std::shared_ptr<CMsg> msg) {
    msg->SetNext(_normal_head);
    msg->SetPrev(nullptr);
    if (_normal_head) {
        _normal_head->SetPrev(msg);
    }
    _normal_head = msg;
    if (!_normal_end) {
        _normal_end = _normal_head;
    }
}

void CFlowQueue::AddToNormalTail(std::shared_ptr<CMsg> msg) {
    msg->SetPrev(_normal_end);
    msg->SetNext(nullptr);
    if (_normal_end) {
        _normal_end->SetNext(msg);
    }
    _normal_end = msg;
    if (_normal_head) {
        _normal_head = _normal_end;
    }
}

void CFlowQueue::AddToResendHead(std::shared_ptr<CMsg> msg) {
    msg->SetNext(_resend_head);
    msg->SetPrev(nullptr);
    if (_resend_head) {
        _resend_head->SetPrev(msg);
    }
    _resend_head = msg;
    if (!_resend_end) {
        _resend_end = _resend_head;
    }
}

void CFlowQueue::AddToResendTail(std::shared_ptr<CMsg> msg) {
    msg->SetPrev(_resend_end);
    msg->SetNext(nullptr);
    if (_resend_end) {
        _resend_end->SetNext(msg);
    }
    _resend_end = msg;
    if (_resend_head) {
        _resend_head = _resend_end;
    }
}
