#include "HudpImpl.h"
#include "HeadFilter.h"

using namespace hudp;

bool CHeadFilter::FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id) {
    // do nothing
    return _next_filter->FilterProcess(handle, flag, body, upper_id);
}

bool CHeadFilter::RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    if (_prev_filter) {
        _prev_filter->RelieveFilterProcess(handle, flag, body);
    } else {
        CHudpImpl::Instance().RecvMessageToUpper(handle, body);
    }
    return true;
}