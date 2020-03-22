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
#include "MsgPoolFactory.h"

using namespace hudp;

// this size may be a dynamic algorithm control
static const uint16_t __send_wnd_size = 5;

CSocketImpl::CSocketImpl(const HudpHandle& handle) : _handle(handle), _sk_status(SS_CLOSE){
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

void CSocketImpl::SendMessage(std::shared_ptr<CMsg> msg) {
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

void CSocketImpl::RecvMessage(std::shared_ptr<CMsg> msg) {
    // get ack info
    GetAckToSendWnd(msg);
    // recv msg to orderlist
    bool done = false;
    uint16_t ret = 0;

    // first time recv msg
    if (_sk_status == SS_CLOSE) {
        StatusChange(SS_READY);
    }

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

    CheckClose(header_flag);

    // normal udp. 
    if (!done && header_flag & HPF_WITH_BODY/*must have body*/) {
        CHudpImpl::Instance().RecvMessageToUpper(_handle, msg);
    }
}

void CSocketImpl::ToRecv(std::shared_ptr<CMsg> msg) {
    // send ack msg to remote
    base::LOG_DEBUG("[receiver] :receiver msg. id : %d", msg->GetId());
    if (!msg->GetBody().empty()) {
        CHudpImpl::Instance().RecvMessageToUpper(_handle, msg);
    }
}

void CSocketImpl::ToSend(std::shared_ptr<CMsg> msg) {
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

void CSocketImpl::AckDone(std::shared_ptr<CMsg> msg) {
    // release msg here
    CTimer::Instance().RemoveTimer(msg);
    msg.reset();
}

void CSocketImpl::TimerOut(std::shared_ptr<CMsg> msg) {
    if (!(msg->GetFlag() & msg_is_only_ack)) {
        // msg resend, increase send delay
        msg->AddSendDelay();

    } else {
        _is_in_timer = false;
    }

    // close now
    if (msg->GetFlag() & msg_wait_2_msl) {
        StatusChange(SS_CLOSE);
        return;
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

void CSocketImpl::AddPendAck(std::shared_ptr<CMsg> msg) {
    auto header_flag = msg->GetHeaderFlag();
    if (header_flag & HTF_RELIABLE_ORDERLY) {
        AddToPendAck(WI_RELIABLE_ORDERLY, msg);

    } else if (header_flag & HTF_RELIABLE) {
        AddToPendAck(WI_RELIABLE, msg);
    }

    // add to timer
    if (!_is_in_timer) {
        std::shared_ptr<CMsg> timer_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
        timer_msg->SetFlag(msg_is_only_ack);
        std::shared_ptr<CSocket> sock = shared_from_this();
        timer_msg->SetSocket(sock);
        timer_msg->SetHandle(_handle);
        CTimer::Instance().AddTimer(__pend_ack_send, timer_msg);
        _is_in_timer = true;
    }
}

void CSocketImpl::AddQuicklyAck(std::shared_ptr<CMsg> msg) {
    std::vector<uint16_t> ack_vec;
    std::vector<uint64_t> time_vec;

    ack_vec.push_back(msg->GetId());
    time_vec.push_back(msg->GetSendTime());

    std::shared_ptr<CMsg> ack_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    ack_msg->SetFlag(msg_is_only_ack);
    std::shared_ptr<CSocket> sock = shared_from_this();
    ack_msg->SetSocket(sock);
    ack_msg->SetHandle(_handle);

    auto header_flag = msg->GetHeaderFlag();
    if (header_flag & HTF_RELIABLE) {
        ack_msg->SetAck(HPF_WITH_RELIABLE_ACK, ack_vec, time_vec, false);

    } else if (header_flag & HTF_RELIABLE_ORDERLY) {
        ack_msg->SetAck(HPF_WITH_RELIABLE_ORDERLY_ACK, ack_vec, time_vec, false);
    }
    // send to net
    CHudpImpl::Instance().SendMessageToNet(ack_msg);
}

void CSocketImpl::SendFinMessage() {
    if (_sk_status != SS_READY && _sk_status != SS_CLOSE_WIAT) {
        base::LOG_ERROR("current status %d can't send a fin msg", _sk_status);
        return;
    }
    std::shared_ptr<CMsg> fin_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    if (__send_all_msg_when_close) {
        fin_msg->SetHeaderFlag(HTF_RELIABLE_ORDERLY | HPF_LOW_PRI | HPF_FIN);

    } else {
        fin_msg->SetHeaderFlag(HTF_RELIABLE_ORDERLY | HPF_HIGHEST_PRI | HPF_FIN);
    }
    std::shared_ptr<CSocket> sock = shared_from_this();
    fin_msg->SetSocket(sock);
    fin_msg->SetHandle(_handle);

    StatusChange(_sk_status == SS_READY ? SS_FIN_WAIT_1 : SS_LAST_ACK);
    SendMessage(fin_msg);
}

bool CSocketImpl::CanSendMessage() {
    return _sk_status == SS_CLOSE || _sk_status == SS_READY;
}

bool CSocketImpl::AddAckToMsg(std::shared_ptr<CMsg> msg) {
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

void CSocketImpl::GetAckToSendWnd(std::shared_ptr<CMsg> msg) {
    // first time recv ack msg
    if (_sk_status == SS_CLOSE) {
        StatusChange(SS_READY);
    }
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

void CSocketImpl::AddToSendWnd(WndIndex index, std::shared_ptr<CMsg> msg) {
    if (!_send_wnd[index]) {
        _send_wnd[index] = new CSendWndImpl(__init_send_wnd_size, CHudpImpl::Instance().CreatePriorityQueue(), 
                index == WI_ORDERLY/*if only orderly, there is no transmission limit*/);
    }
    _send_wnd[index]->PushBack(msg);
}

void CSocketImpl::AddToRecvList(WndIndex index, std::shared_ptr<CMsg> msg) {
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
        AddPendAck(msg);
    }
    _recv_list[index]->Insert(msg);
}

void CSocketImpl::AddToPendAck(WndIndex index, std::shared_ptr<CMsg> msg) {
    if (!_pend_ack[index]) {
        _pend_ack[index] = new CPendAck();
    }
    if (__msg_with_time) {
        _pend_ack[index]->AddAck(msg->GetId(), msg->GetSendTime());

    } else {
        _pend_ack[index]->AddAck(msg->GetId());
    }
}

uint64_t CSocketImpl::GetRtt(std::shared_ptr<CMsg> msg) {
    uint64_t now = GetCurTimeStamp();
    return msg->GetSendTime() - now;
}

uint64_t CSocketImpl::GetRtt(uint64_t time) {
    uint64_t now = GetCurTimeStamp();
    return time - now;
}

void CSocketImpl::StatusChange(socket_status sk_status) {
    switch (sk_status) {
    case hudp::SS_CLOSE:
        // release socket
        CHudpImpl::Instance().ReleaseSocket(_handle);
        break;
    case hudp::SS_READY:
        if (_sk_status == SS_CLOSE) {
            _sk_status = SS_READY;

        } else {
            base::LOG_ERROR("error socket status change. %d to SS_READY", _sk_status);
        }
        break;
    case hudp::SS_FIN_WAIT_1:
        if (_sk_status == SS_READY) {
            _sk_status = SS_FIN_WAIT_1;

        } else {
            base::LOG_ERROR("error socket status change. %d to SS_FIN_WAIT_1", _sk_status);
        }
        break;
    case hudp::SS_FIN_WAIT_2:
        if (_sk_status == SS_FIN_WAIT_1) {
            _sk_status = SS_FIN_WAIT_2;

        } else {
            base::LOG_ERROR("error socket status change. %d to SS_FIN_WAIT_2", _sk_status);
        }
        break;
    case hudp::SS_TIME_WAIT:
        if (_sk_status == SS_FIN_WAIT_2 || _sk_status == SS_CLOSING) {
            _sk_status = SS_TIME_WAIT;
            // add 2MSL timer

        } else {
            base::LOG_ERROR("error socket status change. %d to SS_TIME_WAIT", _sk_status);
        }
        break;
    case hudp::SS_CLOSING:
        if (_sk_status == SS_FIN_WAIT_1) {
            _sk_status = SS_CLOSING;

        } else {
            base::LOG_ERROR("error socket status change. %d to SS_CLOSING", _sk_status);
        }
        break;
    case hudp::SS_CLOSE_WIAT:
        if (_sk_status == SS_READY) {
            _sk_status = SS_CLOSE_WIAT;

        } else {
            base::LOG_ERROR("error socket status change. %d to SS_CLOSE_WIAT", _sk_status);
        }
        break;
    case hudp::SS_LAST_ACK:
        if (_sk_status == SS_CLOSE_WIAT) {
            _sk_status = SS_LAST_ACK;

        } else {
            base::LOG_ERROR("error socket status change. %d to SS_CLOSE_WIAT", _sk_status);
        }
        break;
    default:
        base::LOG_ERROR("unknow socket status. %d", sk_status);
        break;
    }
}

void CSocketImpl::SendFinAckMessage() {
    std::shared_ptr<CMsg> fin_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    // this msg don't need ack
    fin_msg->SetHeaderFlag(HTF_ORDERLY | HPF_HIGHEST_PRI | HPF_FIN_ACK);
    std::shared_ptr<CSocket> sock = shared_from_this();
    fin_msg->SetSocket(sock);
    fin_msg->SetHandle(_handle);
    SendMessage(fin_msg);
}

void CSocketImpl::Wait2MslClose() {
    if (_sk_status != SS_TIME_WAIT) {
        base::LOG_ERROR("error status to add 2 msl timer", _sk_status);
        return;
    }

    std::shared_ptr<CMsg> timer_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    timer_msg->SetFlag(msg_wait_2_msl);
    std::shared_ptr<CSocket> sock = shared_from_this();
    timer_msg->SetSocket(sock);
    timer_msg->SetHandle(_handle);
    CTimer::Instance().AddTimer(__2_msl_time, timer_msg);
}

void CSocketImpl::CheckClose(uint32_t header_flag) {
    // remote start to close connection
    if (header_flag & HPF_FIN) {
        // send fin ack now
        SendFinAckMessage();
        if (_sk_status == SS_READY) {
            StatusChange(SS_CLOSE_WIAT);
            SendFinMessage();

        } else if (_sk_status == SS_FIN_WAIT_1) {
            StatusChange(SS_CLOSING);

        } else if (_sk_status == SS_FIN_WAIT_2) {
            StatusChange(SS_TIME_WAIT);
            Wait2MslClose();

        } else {
            base::LOG_ERROR("error status %d when recv fin", _sk_status);
        }
        return;
    }
    // recv fin ack
    if (header_flag & HPF_FIN_ACK) {
        if (_sk_status == SS_LAST_ACK) {
            StatusChange(SS_CLOSE);

        } else if (_sk_status == SS_FIN_WAIT_1) {
            StatusChange(SS_FIN_WAIT_2);

        } else if (_sk_status == SS_CLOSING) {
            StatusChange(SS_TIME_WAIT);
            Wait2MslClose();

        } else {
            base::LOG_ERROR("error status %d when recv fin ack", _sk_status);
        }
        return;
    }
}