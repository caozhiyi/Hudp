#include <string>
#include <iostream>

#include "Hudp.h"
#include "Runnable.h"

void RecvFunc(const hudp::HudpHandle& handlle, const char* msg, uint16_t len) {
    std::cout << "recv from :" << handlle << " msg : " << msg << std::endl;

    //base::CRunnable::Sleep(5000);
    //CHudp::SendTo(handlle, HTF_RELIABLE_ORDERLY | HPF_HIGHEST_PRI, msg, len);
}

void UtestHudpServer() {
    hudp::Init();

    hudp::Start("127.0.0.1", 8011, RecvFunc);
    
    int index = 0;   

    while (1)
    {
        index++;
        std::string msg = "a test msg. id : " + std::to_string(index);
        
        base::CRunnable::Sleep(300);
        if (index == 20) {
            hudp::Close("127.0.0.1:8012");
            
        } else if (index < 20) {
            hudp::SendTo("127.0.0.1:8012", hudp::HTF_RELIABLE_ORDERLY | hudp::HPF_LOW_PRI, msg);
        }
    }

    hudp::Join();
}

int main() {
    UtestHudpServer();
}
