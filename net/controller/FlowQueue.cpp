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

    std::unique_lock<std::mutex> lock_normal(_normal_mutex);
    std::unique_lock<std::mutex> lock_resend(_resend_mutex);

    if (!_resend_list.empty()) {
        ret = _resend_list.front();
        _resend_list.pop_front();
        
    } else if (!_normal_list.empty()) {
        ret = _normal_list.front();
        _normal_list.pop_front();
    }
    return ret;
}

void CFlowQueue::Remove(std::shared_ptr<CMsg> msg) {
    std::unique_lock<std::mutex> lock_normal(_normal_mutex);
    std::unique_lock<std::mutex> lock_resend(_resend_mutex);

    RemoveUnLock(msg);
}

void CFlowQueue::RemoveUnLock(std::shared_ptr<CMsg> msg) {
    if (!msg) {
        return;
    }

    for (auto iter = _resend_list.begin(); iter != _resend_list.end(); ++iter) {
        if (*iter == msg) {
            _resend_list.erase(iter);
            return;
        }
    }

    for (auto iter = _normal_list.begin(); iter != _normal_list.end(); ++iter) {
        if (*iter == msg) {
            _normal_list.erase(iter);
            return;
        }
    }
}

void CFlowQueue::AddToNormalHead(std::shared_ptr<CMsg> msg) {
    std::unique_lock<std::mutex> lock(_normal_mutex);
    _normal_list.push_front(msg);
}

void CFlowQueue::AddToNormalTail(std::shared_ptr<CMsg> msg) {
    std::unique_lock<std::mutex> lock(_normal_mutex);
    _normal_list.push_back(msg);
}

void CFlowQueue::AddToResendHead(std::shared_ptr<CMsg> msg) {
    std::unique_lock<std::mutex> lock(_normal_mutex);
    _resend_list.push_front(msg);
}

void CFlowQueue::AddToResendTail(std::shared_ptr<CMsg> msg) {
    std::unique_lock<std::mutex> lock(_normal_mutex);
    _resend_list.push_back(msg);
}
