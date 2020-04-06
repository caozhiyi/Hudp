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

    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _timer_map.find(expiration_time);
    ti->SetNext(nullptr);
    // add to timer map
    if (iter == _timer_map.end()) {
        _timer_map[expiration_time] = ti;

    // add same time
    } else {
        // check same item
        std::shared_ptr<CMsg> tmp = iter->second;
        while (tmp->GetNext()) {
            // the same item
            if (tmp == ti) {
                return _time_tool.GetMsec();
            } else {
                tmp = tmp->GetNext();
            }
        }
        // the same item
        if (tmp == ti) {
            return _time_tool.GetMsec();
        }

        // add to list
        tmp->SetNext(ti);
    }
    ti->SetTimerId(expiration_time);
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
    std::map<uint64_t, std::shared_ptr<CMsg>>::iterator iter;
    std::shared_ptr<CMsg> cur_solt;
    bool timer_out = false;
    while (!_stop) {
        {
            cur_solt.reset();
            timer_out = false;
            std::unique_lock<std::mutex> lock(_mutex);
            if (_timer_map.empty()) {
                _wait_time = __timer_empty_wait;

            } else {
                iter = _timer_map.begin();
                _time_tool.Now();
                cur_solt = iter->second;
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
            if (timer_out && cur_solt && cur_solt->GetTimerId() > 0 && cur_solt->GetTimerId() <= (uint64_t)_time_tool.GetMsec()) {
                while (cur_solt) {
                    timer_vec.push_back(cur_solt);
                    cur_solt = cur_solt->GetNext();
                }
                _timer_map.erase(iter);

            // timer is removed
            } else if (timer_out && cur_solt && cur_solt->GetTimerId() == 0 && iter != _timer_map.end()) {
                _timer_map.erase(iter);
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
