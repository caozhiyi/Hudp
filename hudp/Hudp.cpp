#include "Hudp.h"
#include "HudpImpl.h"

void hudp::Init() {
    CHudpImpl::Instance().Init();
}

bool hudp::Start(const std::string& ip, uint16_t port, const recv_back& func) {
    return CHudpImpl::Instance().Start(ip, port, func);
}

void hudp::Join() {
    CHudpImpl::Instance().Join();
}

void hudp::SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg) {
    CHudpImpl::Instance().SendTo(handle, flag, msg);
}

void hudp::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint16_t len) {
    CHudpImpl::Instance().SendTo(handle, flag, msg, len);
}

void hudp::Close(const HudpHandle& handle) {
    CHudpImpl::Instance().Close(handle);
}
