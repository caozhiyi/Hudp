#include <algorithm>

#include "FilterProcess.h"
#include "FilterInterface.h"
#include "Log.h"
#include "NetMsg.h"
#include "RelialeOrderlyFilter.h"
#include "SerializesFilter.h"

using namespace hudp;

void CFilterProcess::Init() {
    // create all filter.
    CSerializesFilter* serializes_filter = new CSerializesFilter();
    CRelialeOrderlyFilter* reliale_orderly_filter = new CRelialeOrderlyFilter();
}

void CFilterProcess::SendProcess(NetMsg* msg) {
    for (uint8_t i = msg->_phase; i > (uint8_t)0; i--) {
        if (i == msg->_phase) {
            _filer_list[(int)(i - 1)]->OnSend(msg);

        } else {
            base::LOG_INFO("send process loop break, id : %d, phase : %d", msg->_head._id, (int)msg->_phase);
            break;
        }
    }
}

void CFilterProcess::RecvProcess(NetMsg* msg) {
    for (uint8_t i = msg->_phase; i <= (uint8_t)_filer_list.size(); i++) {
        if (i == msg->_phase) {
            _filer_list[(int)(i - 1)]->OnRecv(msg);

        } else {
            base::LOG_INFO("recv process loop break, id : %d, phase : %d", msg->_head._id, (int)msg->_phase);
            break;
        }
    }
}

bool CFilterProcess::Add(CFilterInterface* filter) {
    auto iter = std::find(_filer_list.begin(), _filer_list.end(), filter);
    if (iter != _filer_list.end()) {
        base::LOG_ERROR("current filer is already to attach.");
        return false;
    }
    
    _filer_list.push_back(filter);
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