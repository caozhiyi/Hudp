#include <algorithm> // for find
#include "FilterProcessNoThread.h"
#include "Log.h"
#include "IFilter.h"
#include "IMsg.h"

using namespace hudp;

bool CFilterProcessNoThread::AddFilter(const std::shared_ptr<CFilter>& filter) {
    auto iter = std::find(_filer_vec.begin(), _filer_vec.end(), filter);
    if (iter != _filer_vec.end()) {
        base::LOG_WARN("current filer is already to attach.");
        return false;
    }

    _filer_vec.push_back(filter);
    return true;
}

bool CFilterProcessNoThread::RemoveFilter(const std::shared_ptr<CFilter>& filter) {
    auto iter = std::find(_filer_vec.begin(), _filer_vec.end(), filter);
    if (iter != _filer_vec.end()) {
        _filer_vec.erase(iter);
        return true;
    }
    base::LOG_WARN("can't find the filer in list.");
    return false;
}

void CFilterProcessNoThread::PushSendMsg(CMsg* msg) {
    SendProcess(msg);
    _send_call_back(msg);
}

void CFilterProcessNoThread::SetSendFunc(const std::function<void(CMsg*)>& func) {
    _send_call_back = func;
}

void CFilterProcessNoThread::PushRecvMsg(CMsg* msg) {
    RecvProcess(msg);
    _recv_call_back(msg);
}

void CFilterProcessNoThread::SetRecvFunc(const std::function<void(CMsg*)>& func) {
    _recv_call_back = func;
}

void CFilterProcessNoThread::SendProcess(CMsg* msg) {
    for (size_t i = 0; i < _filer_vec.size(); i++) {
        _filer_vec[i]->FilterProcess(msg);
    }
}

void CFilterProcessNoThread::RecvProcess(CMsg* msg) {
    for (size_t i = 0; i < _filer_vec.size(); i++) {
        _filer_vec[i]->RelieveFilterProcess(msg);
    }
}