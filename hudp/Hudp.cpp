#include "Hudp.h"
#include "HudpImpl.h"

hudp::hudp_error_code hudp::Init() {
    CHudpImpl::Instance().Init();
    return HEC_SUCCESS;
}

hudp::hudp_error_code hudp::Start(const std::string& ip, uint16_t port, const recv_back& recv_func, 
                                  const can_write_back& can_write_func) {
    if (CHudpImpl::Instance().Start(ip, port, recv_func, can_write_func)) {
        return HEC_SUCCESS;
    }
    return HEC_FAILED;
}

hudp::hudp_error_code hudp::Join() {
    CHudpImpl::Instance().Join();
    return HEC_SUCCESS;
}

hudp::hudp_error_code hudp::SendTo(const HudpHandle& handle, uint16_t flag, std::string& msg) {
    auto ret = CHudpImpl::Instance().CheckCanSend(handle);
    if (ret != HEC_SUCCESS) {
        return ret;
    }
    
    CHudpImpl::Instance().SendMsgToFilter(handle, flag, msg);
    return HEC_SUCCESS;
}

hudp::hudp_error_code hudp::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint32_t len) {
    auto ret = CHudpImpl::Instance().CheckCanSend(handle);
    if (ret != HEC_SUCCESS) {
        return ret;
    }
    
    std::string send_msg(msg, len);
    CHudpImpl::Instance().SendMsgToFilter(handle, flag, send_msg);
    return HEC_SUCCESS;
}

hudp::hudp_error_code hudp::Close(const HudpHandle& handle) {
    return CHudpImpl::Instance().Close(handle);
}
