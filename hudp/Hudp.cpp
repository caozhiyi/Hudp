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

bool hudp::SendTo(const HudpHandle& handle, uint16_t flag, std::string& msg) {
    return CHudpImpl::Instance().SendMsgToFilter(handle, flag, msg);
}

bool hudp::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint32_t len) {
    std::string send_msg(msg, len);
    return CHudpImpl::Instance().SendMsgToFilter(handle, flag, send_msg);
}

void hudp::Close(const HudpHandle& handle) {
    CHudpImpl::Instance().Close(handle);
}
