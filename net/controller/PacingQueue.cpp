#include "IMsg.h"
#include "HudpImpl.h"
#include "OsCommon.h"
#include "CommonFunc.h"
#include "HudpConfig.h"
#include "PacingQueue.h"

using namespace hudp;

CTimer CPacingQueue::_timer;

CPacingQueue::CPacingQueue() :_next_time(0) {
    
}

CPacingQueue::~CPacingQueue() {
   
}

void CPacingQueue::SetPacingRate(uint32_t pacing_rate) {
    _pacing_rate = pacing_rate;
}

uint32_t CPacingQueue::GetPacingRate() {
    return _pacing_rate;
}

void CPacingQueue::SendMessage(std::shared_ptr<CMsg> msg) {
    // start timer
    static bool do_once = true;
    if (do_once) {
        do_once = false;
        _timer.Start();
    }
    // msg without body, send now.
    if (msg->GetBody().empty()) {
        CHudpImpl::Instance().SendMessageToNet(msg);
        return;
    }

    while (1) {
        uint64_t now = GetCurTimeStamp();

        // add to timer
        if (now < _next_time) {
            msg->SetFlag(msg_pacing_send);
            _timer.AddTimer(_next_time, msg);
            return;

        } else {
            CHudpImpl::Instance().SendMessageToNet(msg);
            return;
        }

        // calc next send time
        uint32_t rate = _pacing_rate ? _pacing_rate : __defaule_pacing_rate;
        uint32_t msg_len = msg->GetEstimateSize();
        msg_len = (uint64_t)msg_len * USEC_PER_MSEC;
        if (rate) {
            msg_len = msg_len / rate;
        }
        // set next send time
        _next_time = msg_len + now;
    }
}

void CPacingQueue::RemoveMsg(std::shared_ptr<CMsg> msg) {
    _timer.RemoveTimer(msg);
}