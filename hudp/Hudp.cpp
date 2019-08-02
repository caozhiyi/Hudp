#include "Hudp.h"
#include "HudpImpl.h"

void hudp::Init(bool log) {
    CHudpImpl::Instance().Init(log);
}

bool hudp::Start(uint16_t port, const recv_back& func) {
    return CHudpImpl::Instance().Start(port, func);
}

void hudp::Join() {
    CHudpImpl::Instance().Join();
}

void hudp::SendTo(const HudpHandle& handlle, uint16_t flag, const std::string& msg) {
    CHudpImpl::Instance().SendTo(handlle, flag, msg);
}

void hudp::SendTo(const HudpHandle& handlle, uint16_t flag, const char* msg, uint16_t len) {
    CHudpImpl::Instance().SendTo(handlle, flag, msg, len);
}

void hudp::Destroy(const HudpHandle& handlle) {
    CHudpImpl::Instance().Destroy(handlle);
}