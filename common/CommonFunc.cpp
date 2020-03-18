#include <random>

#include "TimeTool.h"
#include "CommonFunc.h"

namespace hudp {
    std::pair<uint16_t, std::string> SplitIpPort(const std::string& ip_port) {
        std::pair<uint16_t, std::string> ret;
        ret.first = 0;
        auto pos = ip_port.find(":");
        if (pos == std::string::npos) {
            return ret;
        }

        ret.first = atoi(ip_port.substr(pos + 1, ip_port.length()).c_str());
        ret.second = ip_port.substr(0, pos);

        return std::move(ret);
    }

    uint16_t GetRandomInitialValue() {
        static std::default_random_engine __engine;
        std::uniform_int_distribution<unsigned> u(0, __max_id);
        int ret = u(__engine);
        return ret;
    }

    uint64_t GetCurTimeStamp() {
        base::CTimeTool __time_tool;
        return (uint64_t)__time_tool.GetMsec();
    }

}
