#include "HudpImpl.h"
#include "EndFilter.h"

using namespace hudp;

bool CEndFilter::FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    // send to hudp
    return CHudpImpl::Instance().SendTo(handle, flag, body);
}

bool CEndFilter::RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    // do nothing
    return _prev_filter->RelieveFilterProcess(handle, flag, body);
}