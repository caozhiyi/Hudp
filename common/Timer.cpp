#include "Timer.h"
#include "Log.h"
#include "TimerSolt.h"

#include <iostream>

using namespace hudp;

static const uint16_t __notimer_sleep = 20;
base::CTimeTool CTimer::_time_tool;

inline bool is_between_in(const uint16_t& num, const uint16_t& prev, const uint16_t& next) {
    return num > prev && num <= next;
}

CTimer::CTimer() {
    memset(_timer_list, 0, sizeof(_timer_list));
}

CTimer::~CTimer() {
    Stop();
    AddTimer(1, nullptr);
    Join();
}

void CTimer::AddTimer(uint16_t ms, CTimerSolt* ti) {
    ms = RoundedBackwards(ms);
    _time_tool.Now();

    uint64_t expiration_time = ms + _time_tool.GetMsec();
    uint16_t index = GetIndex(ms);

    std::unique_lock<std::mutex> lock(_mutex);
    CTimerSolt** timer = &(_timer_list[index]);
    if (*timer) {
        ti->SetNext(*timer);
        *timer = ti;
    } else {
        *timer = ti;
    }
    _expiration_timer_map[expiration_time] = ms;
    _notify.notify_one();
}

void CTimer::Run() {
    uint64_t sleep_time = 0;
    std::map<uint64_t, uint16_t>::iterator iter;
    uint16_t index = 0;
    while (!_stop) {
        {
            index = 0;
            sleep_time = 0;

            std::unique_lock<std::mutex> lock(_mutex);
            _notify.wait(_mutex, [this]() {return !this->_expiration_timer_map.empty(); });

            iter = _expiration_timer_map.begin();
            _time_tool.Now();
            if (iter->first > _time_tool.GetMsec()) {
                sleep_time = iter->first - _time_tool.GetMsec();

            } else {
                sleep_time = 0;
            }
        }

        base::CRunnable::Sleep(sleep_time);

        CTimerSolt* ti = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            index = GetIndex(iter->second);

            ti = _timer_list[index];

            _expiration_timer_map.erase(iter);
            _timer_list[index] = nullptr;
        }
        while (ti) {
            ti->OnTimer();
            ti = ti->GetNext();
        }
    }
}

uint16_t CTimer::RoundedBackwards(uint16_t ms) {
    if (is_between_in(ms, 0, 100)) {
        return ms;
    }
    if (is_between_in(ms, 100, 300)) {
        return RoundedBackwards(ms, 2);
    }

    if (is_between_in(ms, 300, 600)) {
        return RoundedBackwards(ms, 4);
    }

    if (is_between_in(ms, 600, 1000)) {
        return RoundedBackwards(ms, 8);
    }

    if (is_between_in(ms, 1000, 5000)) {
        return RoundedBackwards(ms, 32);
    }

    base::LOG_ERROR("timer only support 5s, shouldn't be here.");
    return 0;
}

uint16_t CTimer::RoundedBackwards(uint16_t ms, uint16_t len) {
    return ((ms + len - 1) & ~(len - 1));
}

uint16_t CTimer::GetIndex(uint16_t ms) {
    if (is_between_in(ms, 0, 100)) {
        return ms;
    }
    if (is_between_in(ms, 100, 300)) {
        return 100 + ((ms - 100) >> 1);
    }

    if (is_between_in(ms, 300, 600)) {
        return 200 + ((ms - 300) >> 2);
    }

    if (is_between_in(ms, 600, 1000)) {
        return 300 + ((ms - 600) >> 3);
    }

    if (is_between_in(ms, 1000, 5000)) {
        return 400 + ((ms - 1000) >> 5);
    }

    base::LOG_ERROR("timer only support 5s, shouldn't be here.");
    return 0;
}