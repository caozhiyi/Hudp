#ifndef HEADER_COMMON_TIMER
#define HEADER_COMMON_TIMER

#include <set>
#include "NetMsg.h"

namespace hudp {

    // the timer array length
    static const uint16_t __timer_max_length = 500;

    class CTimerInterface {
    public:
        CTimerInterface() : _data(nullptr), _next(nullptr) {}
        virtual ~CTimerInterface() {}
        // the timer call back
        virtual void OnTimer(void* pt) = 0;
        
        // attach to timer
        void Attach(CTimer timer, uint16_t ms);

        // get next item
        CTimerInterface* GetNext() {
            if (_next) {
                return _next;
            }
            return nullptr;
        }
    private:
        void* _data;
        CTimerInterface *_next;
    };

    // the timer only support timing of up to five seconds.
    // in 1~100ms, every 1 microsecond can set a timer event.
    // in 100~300ms, every 2 microsecond can set a timer event.
    // in 300~600ms, every 3 microsecond can set a timer event.
    // in 600~1000ms, every 4 microsecond can set a timer event.
    // in 1000~5000ms, every 40 microsecond can set a timer event.
    // the more backward the rougher, each input is rounded backwards
    // like 101ms will be set to 102ms, and 602ms will be set to 604ms.
    class CTimer {
    public:
        void AddTimer(uint16_t ms, CTimerInterface* ti);

    private:
        // ms need rounded backwards.
        uint16_t RoundedBackwards(uint16_t ms);
        uint16_t RoundedBackwards(uint16_t ms, uint16_t len);
        // ms to index in array
        uint16_t GetIndex(uint16_t ms);
        
    private:
        CTimerInterface*    _timer_list[__timer_max_length];
        std::set<uint64_t>  _expiration_timer_set;
        uint16_t            _next_up_time;
    };
}

#endif