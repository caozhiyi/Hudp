#include "IMsg.h"
#include "Pacing.h"
#include "HudpImpl.h"
#include "OsCommon.h"
#include "CommonFunc.h"
#include "HudpConfig.h"

using namespace hudp;

CTimer CPacing::_timer;

CPacing::CPacing(std::function<void(std::shared_ptr<CMsg>)> cb) :
            _next_time(0),
            _msg_send_call_back(cb) {
    
}

CPacing::~CPacing() {
   
}

void CPacing::SetPacingRate(uint32_t pacing_rate) {
    _pacing_rate = pacing_rate;
}

uint32_t CPacing::GetPacingRate() {
    return _pacing_rate;
}

void CPacing::SendMessage(std::shared_ptr<CMsg> msg) {
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

    uint64_t now = GetCurTimeStamp();

    // add to timer
    if (now < _next_time) {
        msg->SetFlag(msg_pacing_send);
        _timer.AddTimer((uint32_t)_next_time, msg);
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

    // set send msg time
    msg->SetSendTime(now);
    _msg_send_call_back(msg);
}

void CPacing::RemoveMsg(std::shared_ptr<CMsg> msg) {
    _timer.RemoveTimer(msg);
}