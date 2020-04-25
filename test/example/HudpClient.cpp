#include <string>
#include <iostream>

#include "Hudp.h"
#include "Runnable.h"

using namespace hudp;

uint32_t __global_upper_id = 0;
void RecvFunc(const hudp::HudpHandle& handlle, const char* msg, uint16_t len, hudp_error_code err) {
    if (err == hudp::HEC_SUCCESS) {
        std::cout << "recv from :" << handlle << " msg : " << msg << std::endl;

        return;
    }
    std::cout << "recv from :" << handlle << " err : " << (uint16_t)err << std::endl;
}

void SendFunc(const HudpHandle& handle, uint32_t upper_id, hudp_error_code err) {
    if (err == hudp::HEC_SUCCESS) {
        std::cout << "send success to :" << handle << " upper_id : " << upper_id << std::endl;
    }
}

void UtestHudpServer() {
    hudp::Init();

    hudp::Start("127.0.0.1", 8011, RecvFunc, SendFunc);

    int index = 0;

    while (1)
    {
        index++;
        std::string msg = "a test msg. id : " + std::to_string(index);

        base::CRunnable::Sleep(300);
        if (index == 20) {
            hudp::Close("127.0.0.1:8012");

        }
        else if (index < 20) {
            hudp::SendTo("127.0.0.1:8012", hudp::HTF_RELIABLE_ORDERLY | hudp::HPF_LOW_PRI, msg, ++__global_upper_id);
        }
    }

    hudp::Join();
}

int main() {
    UtestHudpServer();
}
