#ifndef HEADER_COMMON_TIMER
#define HEADER_COMMON_TIMER

#include <condition_variable>
#include <mutex>
#include <map>
#include "Runnable.h"
#include "TimeTool.h"

namespace hudp {

    class CTimerSolt;
    // the timer array length
    static const uint16_t __timer_max_length = 500;

    // the timer only support timing of up to five seconds.
    // in 1~100ms, every 1 microsecond can set a timer event.
    // in 100~300ms, every 2 microsecond can set a timer event.
    // in 300~600ms, every 3 microsecond can set a timer event.
    // in 600~1000ms, every 4 microsecond can set a timer event.
    // in 1000~5000ms, every 40 microsecond can set a timer event.
    // the more backward the rougher, each input is rounded backwards
    // like 101ms will be set to 102ms, and 602ms will be set to 604ms.
    class CTimer : public base::CRunnable {
    public:
        CTimer();
        ~CTimer();
        // add a timer event
        void AddTimer(uint16_t ms, CTimerSolt* ti);
        // thread func 
        virtual void Run();

    private:
        // ms need rounded backwards.
        uint16_t RoundedBackwards(uint16_t ms);
        uint16_t RoundedBackwards(uint16_t ms, uint16_t len);
        // ms to index in array
        uint16_t GetIndex(uint16_t ms);
        
    private:
        // timer event list
        CTimerSolt*                  _timer_list[__timer_max_length];
        // all expiration in list
        std::map<uint64_t, uint16_t> _expiration_timer_map;
        // get now time tool
        static base::CTimeTool       _time_tool;

        // thread safe
        std::mutex					 _mutex;
        std::condition_variable_any	 _notify;
    };
}

#endif