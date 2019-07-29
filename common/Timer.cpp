#include "Timer.h"
#include "Log.h"
#include "TimerSolt.h"

#include <iostream>

using namespace hudp;

#define EMPTY_WAIT  60000 // timer is empty. wait 1 min 

CTimer::CTimer() : _wait_time(EMPTY_WAIT) {
    
}

CTimer::~CTimer() {
    Stop();
    _notify.notify_one();
    Join();
}

uint64_t CTimer::AddTimer(uint16_t ms, CTimerSolt* ti) {
    _time_tool.Now();

    uint64_t expiration_time = ms + _time_tool.GetMsec();

    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _timer_map.find(expiration_time);
    // add to timer map
    if (iter == _timer_map.end()) {
        ti->SetNext(nullptr);
        _timer_map[expiration_time] = ti;

    // add same time
    } else {
        // check same item
        CTimerSolt* tmp = iter->second;
        while (tmp) {
            // the same item
            if (tmp == ti) {
                return 0;
            } else {
                tmp = tmp->GetNext();
            }
        }
        ti->SetNext(iter->second);
        iter->second = ti;
    }
    // wake up timer thread
    if (ms < _wait_time) {
        _notify.notify_one();
    }

    return expiration_time;
}

void CTimer::RemoveTimer(CTimerSolt* ti) {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _timer_map.erase(ti->_timer_id);
    }
    _notify.notify_one();
    
}

void CTimer::Run() {
    std::vector<CTimerSolt*> timer_vec;
    std::map<uint64_t, CTimerSolt*>::iterator iter;
    bool timer_out = false;
    while (!_stop) {
        {
            timer_out = false;
            std::unique_lock<std::mutex> lock(_mutex);
            if (_timer_map.empty()) {
                _wait_time = EMPTY_WAIT;

            } else {
                iter = _timer_map.begin();
                _time_tool.Now();
                if (iter->first > _time_tool.GetMsec()) {
                    _wait_time = iter->first - _time_tool.GetMsec();

                } else {
                    _wait_time = 0;
                    timer_out = true;
                }
            }
            if (_wait_time > 0) {
                timer_out = _notify.wait_for(lock, std::chrono::milliseconds(_wait_time)) == std::cv_status::timeout;
            }

            _time_tool.Now();
            
            // if timer out
            if (timer_out && iter->first <= _time_tool.GetMsec()) {
                while (iter->second) {
                    timer_vec.push_back(iter->second);
                    iter->second = iter->second->GetNext();
                }
                _timer_map.erase(iter);
            }
        }

        if (timer_vec.size() > 0) {
            for (size_t i = 0; i < timer_vec.size(); i++) {
                timer_vec[i]->OnTimer();
            }
            timer_vec.clear();
        }
    }
}