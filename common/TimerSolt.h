#ifndef HEADER_COMMON_TIMERSOLT
#define HEADER_COMMON_TIMERSOLT

#include "Timer.h"
#include "CommonType.h"

namespace hudp {

    // timer event interface
    class CTimerSolt {
    public:
        CTimerSolt() : _next(nullptr), _timer_id(0) {}
        virtual ~CTimerSolt() {}

        //*********************
        // the timer call back
        //*********************
        virtual void OnTimer() = 0;
        
        // attach to timer
        void Attach(uint16_t ms) {
            CTimer::Instance().AddTimer(ms, this);
        }

        CTimerSolt* GetNext() {
            if (_next) {
                return _next;
            }
            return nullptr;
        }
        void SetNext(CTimerSolt* ti) {
            _next = ti;
        }
       
        void Clear() {
            _next = nullptr;
            CTimer::Instance().RemoveTimer(this);
        }

    private:
        friend CTimer;
        CTimerSolt *_next;
        uint64_t    _timer_id;
    };
}

#endif