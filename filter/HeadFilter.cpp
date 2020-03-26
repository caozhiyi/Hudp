#include "HudpImpl.h"
#include "HeadFilter.h"

using namespace hudp;

bool CHeadFilter::FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    // do nothing
    return _next_filter->FilterProcess(handle, flag, body);
}

bool CHeadFilter::RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    CHudpImpl::Instance().RecvMessageToUpper(handle, body);
    return true;
}