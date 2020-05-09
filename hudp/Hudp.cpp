#include "Hudp.h"
#include "HudpImpl.h"

hudp::hudp_error_code hudp::Init() {
    return CHudpImpl::Instance().Init();
}

hudp::hudp_error_code hudp::Start(const std::string& ip, uint16_t port, const recv_back& recv_func, 
                                  const send_back& write_func) {
    if (CHudpImpl::Instance().Start(ip, port, recv_func, write_func)) {
        return HEC_SUCCESS;
    }
    return HEC_FAILED;
}

void hudp::SetConnectCallBack(const connect_back& conn_func) {
    CHudpImpl::Instance().SetConnectCallBack(conn_func);
}

void hudp::SetResendCallBack(const resend_back& resend_func) {
    CHudpImpl::Instance().SetResendCallBack(resend_func);
}

hudp::hudp_error_code hudp::Join() {
    CHudpImpl::Instance().Join();
    return HEC_SUCCESS;
}

hudp::hudp_error_code hudp::SendTo(const HudpHandle& handle, uint16_t flag, std::string& msg,  uint32_t upper_id) {
    auto ret = CHudpImpl::Instance().CheckCanSend(handle);
    if (ret != HEC_SUCCESS) {
        return ret;
    }
    
    CHudpImpl::Instance().SendMsgToFilter(handle, flag, msg, upper_id);
    return HEC_SUCCESS;
}

hudp::hudp_error_code hudp::SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint32_t len,  uint32_t upper_id) {
    auto ret = CHudpImpl::Instance().CheckCanSend(handle);
    if (ret != HEC_SUCCESS) {
        return ret;
    }
    
    std::string send_msg(msg, len);
    CHudpImpl::Instance().SendMsgToFilter(handle, flag, send_msg, upper_id);
    return HEC_SUCCESS;
}

hudp::hudp_error_code hudp::Close(const HudpHandle& handle) {
    return CHudpImpl::Instance().Close(handle);
}
