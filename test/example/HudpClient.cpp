#include "Hudp.h"
#include <iostream>
#include <string>
#include "Runnable.h"

void RecvFunc(const hudp::HudpHandle& handlle, const char* msg, uint16_t len) {
    std::cout << "recv from :" << handlle << " msg : " << msg << std::endl;

    //base::CRunnable::Sleep(5000);
    hudp::SendTo(handlle, hudp::HTF_ORDERLY | hudp::HPF_HIGHEST_PRI, msg, len);
}

void UtestHudpServer() {
    hudp::Init();
    hudp::Start("127.0.0.1", 8012, RecvFunc);

    //base::CRunnable::Sleep(3000);
    //CHudp::SendTo("192.168.1.7:8011", HTF_NORMAL, "a normal msg.");

    hudp::Join();
}

int main() {
    UtestHudpServer();
}
