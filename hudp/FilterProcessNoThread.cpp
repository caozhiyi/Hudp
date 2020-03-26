#include <algorithm> // for find

#include "Log.h"
#include "IMsg.h"
#include "IFilter.h"
#include "FilterProcessNoThread.h"

using namespace hudp;
CFilterProcessNoThread::CFilterProcessNoThread() : 
            _filter_head(nullptr), 
            _filter_end(nullptr) {

}

CFilterProcessNoThread::~CFilterProcessNoThread() {
    while (_filter_head) {
        auto temp = _filter_head;
        _filter_head = _filter_head->GetNextFilter();
        temp.reset();
    }
}

void CFilterProcessNoThread::AddFilter(std::shared_ptr<CFilter> filter) {
    filter->SetPrevFilter(_filter_end);
    filter->SetNextFilter(nullptr);
    if (_filter_end) {
        _filter_end->SetNextFilter(filter);
    }
    _filter_end = filter;
    if (_filter_head) {
        _filter_head = filter;
    }
}

bool CFilterProcessNoThread::PushSendMsg(const HudpHandle& handle, uint16_t flag, std::string& body) {
    return _filter_head->FilterProcess(handle, flag, body);
}

bool CFilterProcessNoThread::PushRecvMsg(const HudpHandle& handle, uint16_t flag, std::string& body) {
    return _filter_end->RelieveFilterProcess(handle, flag, body);
}