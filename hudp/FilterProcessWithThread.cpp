#include <algorithm> // for find

#include "Log.h"
#include "IMsg.h"
#include "IFilter.h"
#include "FilterProcessWithThread.h"

using namespace hudp;
CFilterProcessWithThread::CFilterProcessWithThread() : 
            _filter_head(nullptr), 
            _filter_end(nullptr) {
    // start this thread
    Start();
}

CFilterProcessWithThread::~CFilterProcessWithThread() {
    while (_filter_head) {
        auto temp = _filter_head;
        _filter_head = _filter_head->GetNextFilter();
        temp.reset();
    }
    Stop();
    std::string body;
    PushSendMsg("", 0, body);
    Join();
}

void CFilterProcessWithThread::AddFilter(std::shared_ptr<CFilter> filter) {
    filter->SetPrevFilter(_filter_end);
    filter->SetNextFilter(nullptr);
    if (_filter_end) {
        _filter_end->SetNextFilter(filter);
    }
    _filter_end = filter;
    if (!_filter_head) {
        _filter_head = filter;
    }
}

bool CFilterProcessWithThread::PushSendMsg(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id) {
    FilterProcessParam param(FilterProcessParam::FILTER_SEND, handle, flag, std::move(body), upper_id);
    Push(std::move(param));
    return true;
}

bool CFilterProcessWithThread::PushRecvMsg(const HudpHandle& handle, uint16_t flag, std::string& body) {
    FilterProcessParam param(FilterProcessParam::FILTER_RECV, handle, flag, std::move(body));
    Push(std::move(param));
    return true;
}

void CFilterProcessWithThread::Run() {
    while (!_stop) {
        auto filter_param = _Pop();
        // msg not free to pool
        if (filter_param._filter_type == FilterProcessParam::FILTER_RECV) {
            _filter_end->RelieveFilterProcess(filter_param._handle, filter_param._flag, filter_param._body);

        } else if (filter_param._filter_type == FilterProcessParam::FILTER_SEND) {
            _filter_head->FilterProcess(filter_param._handle, filter_param._flag, filter_param._body, filter_param._upper_id);

        } else {
            base::LOG_WARN("unknow filer param type.");
        }
    }
}