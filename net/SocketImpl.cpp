#include <cstring>		//for memset

#include "Log.h"
#include "IMsg.h"
#include "Timer.h"
#include "SendWnd.h"
#include "PendAck.h"
#include "HudpImpl.h"
#include "SocketImpl.h"
#include "HudpConfig.h"
#include "CommonFlag.h"
#include "CommonFunc.h"
#include "OrderListImpl.h"
#include "PriorityQueue.h"

using namespace hudp;

// this size may be a dynamic algorithm control
static const uint16_t __send_wnd_size = 5;

CSocketImpl::CSocketImpl(const HudpHandle& handle) : _handle(handle) {
    memset(_send_wnd, 0, sizeof(_send_wnd));
    memset(_recv_list, 0, sizeof(_recv_list));
    memset(_pend_ack, 0, sizeof(_pend_ack));
    _is_in_timer.store(false);
}

CSocketImpl::~CSocketImpl() {
    for (uint16_t i = 0; i < __wnd_size; i++) {
        if (_send_wnd[i]) {

            delete _send_wnd[i];
        }
        if (_recv_list[i]) {
            delete _recv_list[i];
        }
        if (_pend_ack[i]) {
            delete _pend_ack[i];
        }
    }
}

HudpHandle CSocketImpl::GetHandle() {
    return _handle;
}

void CSocketImpl::SendMessage(CMsg* msg) {
    auto header_flag = msg->GetHeaderFlag();
    if (header_flag & HTF_ORDERLY) {
        AddToSendWnd(WI_ORDERLY, msg);

    } else if (header_flag & HTF_RELIABLE) {
        AddToSendWnd(WI_RELIABLE, msg);

    } else if (header_flag & HTF_RELIABLE_ORDERLY) {
        AddToSendWnd(WI_RELIABLE_ORDERLY, msg);

    } else {
        // add ack info incidentally
        AddAckToMsg(msg);
        CHudpImpl::Instance().SendMessageToNet(msg);
    }
}

void CSocketImpl::RecvMessage(CMsg* msg) {
    // get ack info
    GetAckToSendWnd(msg);
    // recv msg to orderlist
    bool done = false;
    uint16_t ret = 0;

    auto header_flag = msg->GetHeaderFlag();
    // reliable and orderly
    if (header_flag & HTF_RELIABLE_ORDERLY) {
        AddToRecvList(WI_RELIABLE_ORDERLY, msg);
        done = true;

    // only orderly
    } else if (header_flag & HTF_ORDERLY) {
        AddToRecvList(WI_ORDERLY, msg);
        done = true;

    // only reliable
    } else if (header_flag & HTF_RELIABLE) {
        AddToRecvList(WI_RELIABLE, msg);
        done = true;
    }

    // normal udp. 
    if (!done && header_flag & HPF_WITH_BODY/*must have body*/) {
        CHudpImpl::Instance().RecvMessageToUpper(_handle, msg);
    }
}

void CSocketImpl::ToRecv(CMsg* msg) {
    // send ack msg to remote
    base::LOG_DEBUG("[receiver] :receiver msg. id : %d", msg->GetId());
    CHudpImpl::Instance().RecvMessageToUpper(_handle, msg);
}

void CSocketImpl::ToSend(CMsg* msg) {
    // add to timer
    if (msg->GetHeaderFlag() & HTF_RELIABLE_ORDERLY || msg->GetHeaderFlag() & HTF_RELIABLE) {
        CTimer::Instance().AddTimer(msg->GetReSendTime(), msg);
    }

    // set send msg time
    msg->SetSendTime(GetCurTimeStamp());
    
    // add ack info incidentally
    AddAckToMsg(msg);
    CHudpImpl::Instance().SendMessageToNet(msg);
}

void CSocketImpl::AckDone(CMsg* msg) {
    // release msg here
    CHudpImpl::Instance().ReleaseMessage(msg);
}

void CSocketImpl::TimerOut(CMsg* msg) {
    if (!(msg->GetFlag() & msg_is_only_ack)) {
        // msg resend, increase send delay
        msg->AddSendDelay();

    } else {
        _is_in_timer = false;
    }

    // add ack 
    bool add_ack = AddAckToMsg(msg);
    // not need send ack msg
    if (!add_ack && msg->GetFlag() & msg_is_only_ack) {
        return;
    }

    // set send msg time
    msg->SetSendTime(GetCurTimeStamp());
    // send to net
    CHudpImpl::Instance().SendMessageToNet(msg);
}

void CSocketImpl::AddAck(CMsg* msg) {
    auto header_flag = msg->GetHeaderFlag();
    if (header_flag & HTF_RELIABLE_ORDERLY) {
        AddToPendAck(WI_RELIABLE_ORDERLY, msg);

    } else if (header_flag & HTF_RELIABLE) {
        AddToPendAck(WI_RELIABLE, msg);
    }

    // add to timer
    if (!_is_in_timer) {
        CMsg* timer_msg = CHudpImpl::Instance().CreateMessage();
        timer_msg->SetFlag(msg_is_only_ack);
        std::shared_ptr<CSocket> sock = shared_from_this();
        timer_msg->SetSocket(sock);
        timer_msg->SetHandle(_handle);
        CTimer::Instance().AddTimer(__pend_ack_send, timer_msg);
        _is_in_timer = true;
    }
}

bool CSocketImpl::AddAckToMsg(CMsg* msg) {
    // clear prv ack info
    msg->ClearAck();
    bool ret = false;
    if (_pend_ack[WI_RELIABLE_ORDERLY] && _pend_ack[WI_RELIABLE_ORDERLY]->HasAck()) {
        bool continuity = false;
        std::vector<uint16_t> ack_vec;
        std::vector<uint64_t> time_vec;
        _pend_ack[WI_RELIABLE_ORDERLY]->GetAllAck(ack_vec, time_vec, continuity);
        msg->SetAck(HPF_WITH_RELIABLE_ORDERLY_ACK, ack_vec, time_vec, continuity);
        ret = true;
    }

    if (_pend_ack[WI_RELIABLE] && _pend_ack[WI_RELIABLE]->HasAck()) {
        bool continuity = false;
        std::vector<uint16_t> ack_vec;
        std::vector<uint64_t> time_vec;
        _pend_ack[WI_RELIABLE]->GetAllAck(ack_vec, time_vec, continuity);
        msg->SetAck(HPF_WITH_RELIABLE_ACK, ack_vec, time_vec, continuity);
        ret = true;
    }
    return ret;
}

void CSocketImpl::GetAckToSendWnd(CMsg* msg) {
    if (msg->GetHeaderFlag() & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        std::vector<uint16_t> vec;
        std::vector<uint64_t> time_vec;
        msg->GetAck(HPF_WITH_RELIABLE_ORDERLY_ACK, vec, time_vec);
        for (uint16_t index = 0; index < vec.size(); index++) {
            _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(vec[index]);

            uint64_t rtt_time = 0;
            if (__msg_with_time) {
                rtt_time = GetRtt(time_vec[index]);
                // TODO
                // set rtt sample
                // _rto.SetAckTime(index, time_stap);
            }
        }
        if (!__msg_with_time) {
            uint64_t rtt_time = GetRtt(msg);
            // TODO
            // set rtt sample
            // _rto.SetAckTime(index, time_stap);
        }
    }

    if (msg->GetHeaderFlag() & HPF_WITH_RELIABLE_ACK) {
        std::vector<uint16_t> vec;
        std::vector<uint64_t> time_vec;
        msg->GetAck(HPF_WITH_RELIABLE_ACK, vec, time_vec);
        for (uint16_t index = 0; index < vec.size(); index++) {
            _send_wnd[WI_RELIABLE]->AcceptAck(vec[index]);

            if (__msg_with_time) {
                uint64_t rtt_time = GetRtt(time_vec[index]);
                // TODO
                // set rtt sample
                // _rto.SetAckTime(index, time_stap);
            }
        }
        if (!__msg_with_time) {
            uint64_t rtt_time = GetRtt(msg);
            // TODO
            // set rtt sample
            // _rto.SetAckTime(index, time_stap);
        }
    }
}

void CSocketImpl::AddToSendWnd(WndIndex index, CMsg* msg) {
    if (!_send_wnd[index]) {
        _send_wnd[index] = new CSendWndImpl(__init_send_wnd_size, CHudpImpl::Instance().CreatePriorityQueue(), 
                index == WI_ORDERLY/*if only orderly, there is no transmission limit*/);
    }
    _send_wnd[index]->PushBack(msg);
}

void CSocketImpl::AddToRecvList(WndIndex index, CMsg* msg) {
    if (!_recv_list[index]) {
        uint16_t id = msg->GetId();
        if (index == WI_ORDERLY) {
            _recv_list[index] = new COrderlyList(id);

        } else if (index == WI_RELIABLE) {
            _recv_list[index] = new CReliableList(id);

        } else if (index == WI_RELIABLE_ORDERLY) {
            _recv_list[index] = new CReliableOrderlyList(id);
        }
    }
    // should send ack to remote
    if (index == WI_RELIABLE || index == WI_RELIABLE_ORDERLY) {
        AddAck(msg);
    }
    _recv_list[index]->Insert(msg);
}

void CSocketImpl::AddToPendAck(WndIndex index, CMsg* msg) {
    if (!_pend_ack[index]) {
        _pend_ack[index] = new CPendAck();
    }
    if (__msg_with_time) {
        _pend_ack[index]->AddAck(msg->GetId(), msg->GetSendTime());

    } else {
        _pend_ack[index]->AddAck(msg->GetId());
    }
}

uint64_t CSocketImpl::GetRtt(CMsg* msg) {
    uint64_t now = GetCurTimeStamp();
    return msg->GetSendTime() - now;
}

uint64_t CSocketImpl::GetRtt(uint64_t time) {
    uint64_t now = GetCurTimeStamp();
    return time - now;
}