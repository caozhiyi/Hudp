#ifndef HEADER_NET_CONTROLLER_PACING
#define HEADER_NET_CONTROLLER_PACING

#include <memory>
#include <functional>

#include "Timer.h"

namespace hudp {

class CMsg;
class CPacing {
    public:
    CPacing(std::function<void(std::shared_ptr<CMsg> msg)> cb);
    ~CPacing();

    void SetPacingRate(uint32_t pacing_rate);
    uint32_t GetPacingRate();

    void SendMessage(std::shared_ptr<CMsg> msg);

    void RemoveMsg(std::shared_ptr<CMsg> msg);

    private:
    // pacing queue use other timer 
    static CTimer _timer;
    uint32_t _pacing_rate;
    uint64_t _next_time;
    std::function<void(std::shared_ptr<CMsg> msg)> _msg_send_call_back;
};

}
#endif