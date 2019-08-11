#include "Hudp.h"
#include <iostream>
#include <string>
#include "Runnable.h"

void RecvFunc(const hudp::HudpHandle& handlle, const char* msg, uint16_t len) {
    std::cout << "recv from :" << handlle << " msg : " << msg << std::endl;

    //base::CRunnable::Sleep(5000);
    //CHudp::SendTo(handlle, HTF_RELIABLE_ORDERLY | HPF_HIGHEST_PRI, msg, len);
}

void UtestHudpServer() {
    hudp::Init();

    hudp::Start("192.168.1.4", 8011, RecvFunc);
    
    int index = 0;
    while (1)
    {
        index++;
        std::string msg = "a test msg. id : " + std::to_string(index);
        hudp::SendTo("192.168.1.4:8012", hudp::HTF_RELIABLE_ORDERLY | hudp::HPF_HIGHEST_PRI, msg);
        base::CRunnable::Sleep(1000);
    }

    hudp::Join();
}

int main() {
    UtestHudpServer();
}