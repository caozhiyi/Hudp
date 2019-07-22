#ifndef HEADER_COMMON_TIMERSOLT
#define HEADER_COMMON_TIMERSOLT

#include "Timer.h"

namespace hudp {

    // timer event interface
    class CTimerSolt {
    public:
        CTimerSolt() : _next(nullptr) {}
        virtual ~CTimerSolt() {}

        //*********************
        // the timer call back
        //*********************
        virtual void OnTimer() = 0;
        
        // attach to timer
        void Attach(CTimer* timer, uint16_t ms) {
            timer->AddTimer(ms, this);
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
        }

    private:
        CTimerSolt *_next;
    };
}

#endif