#ifndef HEADER_COMMON_COMMONFUNC
#define HEADER_COMMON_COMMONFUNC

#include <string>
#include "CommonType.h"

namespace hudp {
    // split ip port string. ip port like "127.0.0.1:8080"
    std::pair<uint16_t, std::string> SplitIpPort(const std::string& ip_port);
    // get a random num
    uint16_t GetRandomInitialValue();
}
#endif