#ifndef HEADER_NET_PACINGQUEUE
#define HEADER_NET_PACINGQUEUE

#include <memory>
#include "Timer.h"

namespace hudp {

class CMsg;
class CPacingQueue {
    public:
    CPacingQueue();
    ~CPacingQueue();

    void SetPacingRate(uint32_t pacing_rate);
    uint32_t GetPacingRate();

    void SendMessage(std::shared_ptr<CMsg> msg);

    void RemoveMsg(std::shared_ptr<CMsg> msg);

    private:
    // pacing queue use other timer 
    static CTimer _timer;
    uint32_t _pacing_rate;
    uint64_t _next_time;
};

}
#endif