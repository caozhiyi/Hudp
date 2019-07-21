#include "FilterInterface.h"
#include "FilterProcess.h"
#include "NetMsg.h"
using namespace hudp;

CFilterInterface::CFilterInterface(uint8_t phase) : _handle_phase(phase) {
    Attach();
}

CFilterInterface:: ~CFilterInterface() {
    
}

bool CFilterInterface::Attach() {
    return CFilterProcess::Instance().Add(this);
}

bool CFilterInterface::Relieve() {
    return CFilterProcess::Instance().Remove(this);
}

const uint8_t& CFilterInterface::GetPhase() {
    return _handle_phase;
}

void CFilterInterface::NextPhase(NetMsg* msg) {
    msg->_phase += 1;
}