#include <algorithm>
#include "FilterProcess.h"
#include "FilterInterface.h"
#include "Log.h"

using namespace hudp;

void CFilterProcess::SendProcess(NetMsg* msg) {
    for (size_t i = GetIndex(msg->_phase); i < _filer_list.size(); i++) {
        if (_filer_list[i]->GetPhase() == PP_UPPER_HANDLE && msg->_phase & PP_UPPER_HANDLE) {
            dynamic_cast<CUpperFilterInterface*>(_filer_list[i])->OnSend(msg);
        }
        
        if (_filer_list[i]->GetPhase() == PP_BODY_HANDLE && msg->_phase & PP_BODY_HANDLE) {
            dynamic_cast<CBodyFilterInterface*>(_filer_list[i])->OnSend(msg);
        }

        if (_filer_list[i]->GetPhase() == PP_HEAD_HANDLE && msg->_phase & PP_HEAD_HANDLE) {
            dynamic_cast<CHeadFilterInterface*>(_filer_list[i])->OnSend(msg);
        }

        if (_filer_list[i]->GetPhase() == PP_PROTO_PARSE && msg->_phase & PP_PROTO_PARSE ) {
            dynamic_cast<CProtocolFilterInterface*>(_filer_list[i])->OnSend(msg);
        }
    }
}

void CFilterProcess::RecvProcess(NetMsg* msg) {
    size_t i = _filer_list.size() - GetIndex(msg->_phase);
    for (; i >= 0; i--) {
        if (_filer_list[i]->GetPhase() == PP_PROTO_PARSE && msg->_phase & PP_PROTO_PARSE) {
            dynamic_cast<CProtocolFilterInterface*>(_filer_list[i])->OnRecv(msg);
        }

        if (_filer_list[i]->GetPhase() == PP_HEAD_HANDLE && msg->_phase & PP_HEAD_HANDLE) {
            dynamic_cast<CHeadFilterInterface*>(_filer_list[i])->OnRecv(msg);
        }

        if (_filer_list[i]->GetPhase() == PP_BODY_HANDLE && msg->_phase & PP_BODY_HANDLE) {
            dynamic_cast<CBodyFilterInterface*>(_filer_list[i])->OnRecv(msg);
        }

        if (_filer_list[i]->GetPhase() == PP_UPPER_HANDLE && msg->_phase & PP_UPPER_HANDLE) {
            dynamic_cast<CUpperFilterInterface*>(_filer_list[i])->OnRecv(msg);
        }
    }
}

bool CFilterProcess::Add(CFilterInterface* filter) {
    if (_cur_handle_phase > filter->GetPhase()) {
        base::LOG_ERROR("current filer is too low to attch.");
        return false;
    }
    _filer_list.push_back(filter);
    _cur_handle_phase = filter->GetPhase();
    return true;
}

bool CFilterProcess::Remove(CFilterInterface* filter) {
    auto iter = std::find(_filer_list.begin(), _filer_list.end(), filter);
    if (iter != _filer_list.end()) {
        _filer_list.erase(iter);
        return true;
    }
    base::LOG_ERROR("can't find the filer in list.");
    return false;
}

size_t CFilterProcess::GetIndex(process_phase cur_handle_phase) {
    return 1;
}