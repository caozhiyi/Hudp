#include "CommonFunc.h"

namespace hudp {
    std::pair<uint16_t, std::string> SplitIpPort(const std::string& ip_port) {
        std::pair<uint16_t, std::string> ret;
        ret.first = 0;
        int pos = ip_port.find(":");
        if (pos == std::string::npos) {
            return ret;
        }

        ret.first = atoi(ip_port.substr(pos + 1, ip_port.length()).c_str());
        ret.second = ip_port.substr(0, pos);

        return ret;
    }

}