#include "HudpImpl.h"
#include "UpperThreadFilter.h"

using namespace hudp;

CUpperThreadFilter::CUpperThreadFilter() {
    // start thread
    Start();
}

CUpperThreadFilter::~CUpperThreadFilter() {
    Stop();
    std::string body;
    RelieveFilterProcess("", 0, body);
    Join();
}

bool CUpperThreadFilter::FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id) {
    // do nothing
    return _next_filter->FilterProcess(handle, flag, body, upper_id);
}

bool CUpperThreadFilter::RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    FilterProcessParam param(FilterProcessParam::FILTER_RECV, handle, flag, std::move(body), 0);
    Push(std::move(param));
    return true;
}

void CUpperThreadFilter::Run() {
    while (!_stop) {
        auto filter_param = _Pop();
        if (!filter_param._handle.empty()) {
            CHudpImpl::Instance().RecvMessageToUpper(filter_param._handle, filter_param._body);
        }
    }
}