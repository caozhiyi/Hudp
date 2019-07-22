#include "Hudp.h"
#include <iostream>
#include <string>
#include "Runnable.h"
using namespace hudp;

void RecvFunc(const HudpHandle& handlle, const char* msg, uint16_t len) {
    std::cout << "recv from :" << handlle << " msg : " << msg << std::endl;

    base::CRunnable::Sleep(1000);
    CHudp::SendTo(handlle, HTF_RELIABLE_ORDERLY | HPF_HIGHEST_PRI, msg, len);
}

void UtestHudpServer() {
    CHudp::Init(true);

    CHudp::Start(8011, RecvFunc);
    
    
    CHudp::SendTo("192.168.1.7:8012", HTF_RELIABLE_ORDERLY | HPF_HIGHEST_PRI, "a normal msg.");

    CHudp::Join();
}

int main() {
    UtestHudpServer();
}