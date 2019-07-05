#include "FilterInterface.h"
#include "FilterProcess.h"
using namespace hudp;

bool CFilterInterface::Attach() {
    return CFilterProcess::Instance().Add(this);
}

bool CFilterInterface::Relieve() {
    return CFilterProcess::Instance().Remove(this);
}

const process_phase& CFilterInterface::GetPhase() {
    return _handle_phase;
}