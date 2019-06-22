#include <algorithm>
#include "FilterInterface.h"
#include "Log.h"
using namespace hudp;

std::vector<CFilterInterface*> CFilterInterface::__filer_list;
process_phase CFilterInterface::__cur_handle_phase = PP_PROTO_PARSE;

bool CFilterInterface::Attach() {
    if (__cur_handle_phase > _handle_phase) {
        base::LOG_ERROR("current filer is too low to attch.");
        return false;
    }
    __filer_list.push_back(this);
    __cur_handle_phase = _handle_phase;
    return true;
}

bool CFilterInterface::Relieve() {
    auto iter = std::find(__filer_list.begin(), __filer_list.end(), this);
    if (iter != __filer_list.end()) {
        __filer_list.erase(iter);
        return true;
    }
    base::LOG_ERROR("can't find the filer in list.");
    return false;
}

const process_phase& CFilterInterface::GetPhase() {
    return _handle_phase;
}