#include <string>
#include <iostream>

#include "Hudp.h"
#include "Runnable.h"

using namespace hudp;

uint32_t __global_upper_id = 0;
void RecvFunc(const hudp::HudpHandle& handlle, const char* msg, uint16_t len, hudp_error_code err) {
    if (err == hudp::HEC_SUCCESS) {
        std::cout << "recv from :" << handlle << " msg : " << msg << std::endl;

        //base::CRunnable::Sleep(5000);
        hudp::SendTo(handlle, hudp::HTF_RELIABLE | hudp::HPF_HIGHEST_PRI, msg, len, ++__global_upper_id);
        return;
    }
    std::cout << "recv from :" << handlle << " err : " << (uint16_t)err << std::endl;
}

void SendFunc(const HudpHandle& handle, uint32_t upper_id, hudp_error_code err) {
    if (err == hudp::HEC_SUCCESS) {
        std::cout << "send success to :" << handle << " upper_id : " << upper_id << std::endl;
    }
}

void ResendFunc(const HudpHandle& handle, uint32_t upper_id, bool& continue_send) {
    std::cout << "resend msg :" << handle << " upper_id : " << upper_id << std::endl;
}

void ConnectFunc(const HudpHandle& handle, hudp_error_code err) {
    std::cout << "get a connect from:" << handle << " err : " << (uint32_t)err << std::endl;
}

void UtestHudpServer() {
    hudp::Init();
    hudp::SetConnectCallBack(ConnectFunc);
    hudp::SetResendCallBack(ResendFunc);
    hudp::Start("127.0.0.1", 8012, RecvFunc, SendFunc);

    hudp::Join();
}

int main() {
    UtestHudpServer();
}

