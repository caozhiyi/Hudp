#include "IMsg.h"
#include "Timer.h"
#include "ISocket.h"
#include "HudpConfig.h"

using namespace hudp;

CTimer::CTimer() : _wait_time(__timer_empty_wait) {
    
}

CTimer::~CTimer() {
    Stop();
    _notify.notify_one();
    Join();
}

uint64_t CTimer::AddTimer(uint32_t ms, std::shared_ptr<CMsg> ti) {
    _time_tool.Now();
    uint64_t expiration_time = ms + _time_tool.GetMsec();

    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto iter = _timer_map.find(expiration_time);
        _timer_map[expiration_time].push_back(ti);
        ti->SetTimerId(expiration_time);
    }

    // wake up timer thread
    if (ms < _wait_time) {
        _notify.notify_one();
    }
    return expiration_time;
}

void CTimer::RemoveTimer(std::shared_ptr<CMsg> ti) {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _timer_map.erase(ti->GetTimerId());
        ti->SetTimerId(0);
    }
    _notify.notify_one();
}

void CTimer::RemoveTimer(CMsg* ti) {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _timer_map.erase(ti->GetTimerId());
        ti->SetTimerId(0);
    }
    _notify.notify_one();
}

uint64_t CTimer::GetTimeStamp() {
    _time_tool.Now();
    return _time_tool.GetMsec();
}

void CTimer::Run() {
    std::vector<std::shared_ptr<CMsg>> timer_vec;
    std::list<std::shared_ptr<CMsg>> *cur_list = nullptr;
    uint64_t cur_timestemp = 0;
    bool timer_out = false;

    while (!_stop) {
        {
            cur_list = nullptr;
            timer_out = false;
            std::unique_lock<std::mutex> lock(_mutex);
            if (_timer_map.empty()) {
                _wait_time = __timer_empty_wait;

            } else {
                auto iter = _timer_map.begin();
                cur_timestemp = iter->first;
                _time_tool.Now();
                cur_list = &iter->second;
                if (iter->first > (uint64_t)_time_tool.GetMsec()) {
                    _wait_time = iter->first - _time_tool.GetMsec();

                } else {
                    _wait_time = 0;
                    timer_out = true;
                }
            }
            if (_wait_time > 0) {
                timer_out = _notify.wait_for(lock, std::chrono::milliseconds(_wait_time)) == std::cv_status::timeout;
                 _time_tool.Now();
            }

            // if timer out
            if (timer_out && cur_list && cur_timestemp > 0 && cur_timestemp <= (uint64_t)_time_tool.GetMsec()) {
                while (!cur_list->empty()) {
                    timer_vec.push_back(cur_list->front());
                    cur_list->pop_front();
                }
                _timer_map.erase(cur_timestemp);

            // timer is removed
            } else if (timer_out && cur_list && cur_timestemp == 0) {
                _timer_map.erase(cur_timestemp);
            }
        }

        if (timer_vec.size() > 0) {
            for (size_t i = 0; i < timer_vec.size(); i++) {
                auto sock = timer_vec[i]->GetSocket();
                if (sock) {
                    sock->TimerOut(timer_vec[i]);
                }
            }
            timer_vec.clear();
        }
    }
}
