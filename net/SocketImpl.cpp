#include <cstring>		//for memset

#include "Rto.h"
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
#include "controller/Pacing.h"
#include "controller/bbr/bbr.h"
#include "controller/FlowQueue.h"

using namespace hudp;

// reserved time stamp
static const uint16_t __msg_length_limit = __mtu - sizeof(uint64_t);

CSocketImpl::CSocketImpl(const HudpHandle& handle) : _handle(handle), _sk_status(SS_CLOSE){
    memset(_send_wnd, 0, sizeof(_send_wnd));
    memset(_recv_list, 0, sizeof(_recv_list));
    memset(_pend_ack, 0, sizeof(_pend_ack));
    _is_in_timer.store(false);
    _in_flight.store(0);
    _lost_msg.store(0);
    _pacing.reset(new CPacing(std::bind(&CSocketImpl::PacingCallBack, this, std::placeholders::_1)));
    _flow_queue.reset(new CFlowQueue());
    _rto.reset(new CRtoImpl());
    _bbr_controller.reset(new CBbr());
}

CSocketImpl::~CSocketImpl() {
    _sk_status = SS_CLOSE;
    for (uint16_t i = 0; i < __wnd_size; i++) {
        if (_send_wnd[i]) {
            _send_wnd[i]->Clear();
            delete _send_wnd[i];
        }
        if (_recv_list[i]) {
            _recv_list[i]->Clear();
            delete _recv_list[i];
        }
        if (_pend_ack[i]) {
            _pend_ack[i]->Clear();
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
        SendPacingMsg(msg);
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
        CHudpImpl::Instance().NewConnectToUpper(_handle, HEC_SUCCESS);
        StatusChange(SS_READY);
    }

    // time wait status send rst to remote.
    if (_sk_status == SS_TIME_WAIT) {
        SendResetMsg();
        return; 
    }

    auto header_flag = msg->GetHeaderFlag();

    // if recv rst msg, close this connection now
    if (header_flag & HPF_RST) {
        CHudpImpl::Instance().NewConnectToUpper(_handle, HEC_BREAK);
        // release socket
        _sk_status = SS_CLOSE;
        CHudpImpl::Instance().ReleaseSocket(_handle);
        return;
    }

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
        CHudpImpl::Instance().RecvMsgToFilter(_handle, msg->GetHeaderFlag(), msg->GetBody());
    }
}

void CSocketImpl::ToRecv(std::shared_ptr<CMsg> msg) {
    // send ack msg to remote
    base::LOG_DEBUG("[receiver] :receiver msg. id : %d", msg->GetId());
    if (!msg->GetBody().empty()) {
        CHudpImpl::Instance().RecvMsgToFilter(_handle, msg->GetHeaderFlag(), msg->GetBody());
    }
}

void CSocketImpl::ToSend(std::shared_ptr<CMsg> msg) {
    // add to timer
    if (msg->GetHeaderFlag() & HTF_RELIABLE_ORDERLY || msg->GetHeaderFlag() & HTF_RELIABLE) {
        CTimer::Instance().AddTimer(msg->GetReSendTime(_rto->GetRto()), msg);
    }

    // set send msg time
    msg->SetSendTime(GetCurTimeStamp());
    
    // add ack info incidentally
    AddAckToMsg(msg);
    SendPacingMsg(msg);
}

void CSocketImpl::AckDone(std::shared_ptr<CMsg> msg) {
    // send back upper
    if (msg->GetHeaderFlag() & HPF_UPPER) {
        CHudpImpl::Instance().SendBackToUpper(_handle, msg->GetUpperId(), HEC_SUCCESS);
    }

    // release msg here
    CTimer::Instance().RemoveTimer(msg);
    if (__use_fq_and_pacing) {
       _flow_queue->Remove(msg);
    }
    msg.reset();
}

void CSocketImpl::TimerOut(std::shared_ptr<CMsg> msg) {
    if (msg->GetFlag() & msg_pacing_send) {
        // send to net
        SendPacingMsg(msg, false);
    }

    if (!(msg->GetFlag() & msg_is_only_ack)) {
        // continue resend call back
        bool continue_resend = true;
        CHudpImpl::Instance().ResendBackToUpper(_handle, msg->GetUpperId(), continue_resend);
        if (!continue_resend) {
            return;
        }

        // msg resend, increase send delay
        msg->AddSendDelay();
        msg->SetFlag(msg_resend);
        _lost_msg.fetch_add(1);
       
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

    // resend to net
    SendPacingMsg(msg);
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

    CHudpImpl::Instance().NewConnectToUpper(_handle, HEC_CLOSED);
    StatusChange(_sk_status == SS_READY ? SS_FIN_WAIT_1 : SS_LAST_ACK);
    SendMessage(fin_msg);
}

bool CSocketImpl::CanSendMessage() {
    return _sk_status == SS_CLOSE || _sk_status == SS_READY;
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

bool CSocketImpl::AddAckToMsg(std::shared_ptr<CMsg> msg) {
    // clear prv ack info
    msg->ClearAck();
    bool ret = false;
    if (_pend_ack[WI_RELIABLE_ORDERLY] && _pend_ack[WI_RELIABLE_ORDERLY]->HasAck()) {
        bool continuity = false;
        std::vector<uint16_t> ack_vec;
        std::vector<uint64_t> time_vec;
        _pend_ack[WI_RELIABLE_ORDERLY]->GetAck(ack_vec, time_vec, continuity, __msg_length_limit - msg->GetEstimateSize());
        msg->SetAck(HPF_WITH_RELIABLE_ORDERLY_ACK, ack_vec, time_vec, continuity);
        ret = true;
    }

    if (_pend_ack[WI_RELIABLE] && _pend_ack[WI_RELIABLE]->HasAck()) {
        bool continuity = false;
        std::vector<uint16_t> ack_vec;
        std::vector<uint64_t> time_vec;
        _pend_ack[WI_RELIABLE]->GetAck(ack_vec, time_vec, continuity, __msg_length_limit - msg->GetEstimateSize());
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

    uint64_t rtt_time = 0;
    uint32_t ack_msg_size = 0;
    if (msg->GetHeaderFlag() & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        std::vector<uint16_t> vec;
        std::vector<uint64_t> time_vec;
        msg->GetAck(HPF_WITH_RELIABLE_ORDERLY_ACK, vec, time_vec);
        for (uint16_t index = 0; index < vec.size(); index++) {
            uint32_t ack_msg_size = _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(vec[index]);
            _in_flight.fetch_sub(ack_msg_size);
            
            if (__msg_with_time) {
                rtt_time = GetRtt(time_vec[index]);
                _rto->SetRttTime(rtt_time);
            }
        }
        ControllerProcess(WI_RELIABLE_ORDERLY, (uint32_t)rtt_time, (uint32_t)vec.size(), ack_msg_size);
        if (!__msg_with_time) {
            uint64_t rtt_time = GetRtt(msg);
            _rto->SetRttTime(rtt_time);
        }
    }

    if (msg->GetHeaderFlag() & HPF_WITH_RELIABLE_ACK) {
        std::vector<uint16_t> vec;
        std::vector<uint64_t> time_vec;
        msg->GetAck(HPF_WITH_RELIABLE_ACK, vec, time_vec);
        for (uint16_t index = 0; index < vec.size(); index++) {
            uint32_t ack_msg_size = _send_wnd[WI_RELIABLE]->AcceptAck(vec[index]);
            _in_flight.fetch_sub(ack_msg_size);

            if (__msg_with_time) {
                uint64_t rtt_time = GetRtt(time_vec[index]);
                _rto->SetRttTime(rtt_time);
            }
        }
        ControllerProcess(WI_RELIABLE, (uint32_t)rtt_time, (uint32_t)vec.size(), ack_msg_size);
        if (!__msg_with_time) {
            uint64_t rtt_time = GetRtt(msg);
            _rto->SetRttTime(rtt_time);
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
    
    auto ret = _recv_list[index]->Insert(msg);

    // continuously disordered messages
    if (ret == 2) {

        CHudpImpl::Instance().RecvMessageToUpper(_handle, __empty_str, HEC_BREAK);
        SendResetMsg();
        return;
    }

    // should send ack to remote
    if (index == WI_RELIABLE || index == WI_RELIABLE_ORDERLY) {
        // disorder msg, send ack quickly
        if (ret == 1) {
            AddQuicklyAck(msg);

        } else {
            AddPendAck(msg);
        }
    }
}

void CSocketImpl::AddToPendAck(WndIndex index, std::shared_ptr<CMsg> msg) {
    if (!_pend_ack[index]) {
        _pend_ack[index] = new CPendAck();
        _pend_ack[index]->SetSendAckNowCallBack(std::bind(&CSocketImpl::SendAckNowCallBack, this));
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

void CSocketImpl::SendResetMsg() {
    std::shared_ptr<CMsg> rst_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    // this msg don't need ack
    rst_msg->SetHeaderFlag(HTF_ORDERLY | HPF_HIGHEST_PRI | HPF_RST);
    std::shared_ptr<CSocket> sock = shared_from_this();
    rst_msg->SetSocket(sock);
    rst_msg->SetHandle(_handle);
    SendMessage(rst_msg);
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

void CSocketImpl::SendPacingMsg(std::shared_ptr<CMsg> msg, bool add_fq) {
    if (__use_fq_and_pacing && !(msg->GetFlag() & msg_is_only_ack)) {
        if (add_fq) {
            _flow_queue->Add(msg);
        }
        auto send_msg = _flow_queue->Get();
        if (send_msg) {
            _pacing->SendMessage(send_msg);
        }
        return;
    }
    // set send msg time
    if (!(msg->GetFlag() & msg_is_only_ack)) {
        msg->SetSendTime(GetCurTimeStamp());
    }
    CHudpImpl::Instance().SendMessageToNet(msg);    
}

void CSocketImpl::ControllerProcess(WndIndex index, uint32_t rtt, uint32_t acked, uint32_t delivered) {
    uint64_t now = GetCurTimeStamp();
    bool app_limit = _send_wnd[index] ? false : _send_wnd[index]->IsAppLimit();
    uint32_t send_wnd = _send_wnd[index] ? 0 : _send_wnd[index]->GetWndSize();
    uint32_t pacing = _pacing->GetPacingRate();

    _bbr_controller->bbr_main(_in_flight.load(), rtt, acked, now, delivered, _lost_msg.load(), app_limit, send_wnd, pacing);

    _lost_msg.store(0);
    _send_wnd[index]->ChangeSendWndSize(send_wnd);
    _pacing->SetPacingRate(pacing);
}

void CSocketImpl::PacingCallBack(std::shared_ptr<CMsg> msg) {
    if (msg->GetHeaderFlag() & HTF_RELIABLE || msg->GetHeaderFlag() & HTF_RELIABLE_ORDERLY) {
        _in_flight.fetch_add(msg->GetEstimateSize());
    }
    CHudpImpl::Instance().SendMessageToNet(msg);
}

void CSocketImpl::SendAckNowCallBack() {
    // get a msg
    std::shared_ptr<CMsg> ack_msg = CMsgPoolFactory::Instance().CreateSharedMsg();
    ack_msg->SetFlag(msg_is_only_ack);
    std::shared_ptr<CSocket> sock = shared_from_this();
    ack_msg->SetSocket(sock);
    ack_msg->SetHandle(_handle);

    // add ack 
    bool add_ack = AddAckToMsg(ack_msg);
    // not need send ack msg
    if (!add_ack && ack_msg->GetFlag() & msg_is_only_ack) {
        return;
    }

    // resend to net
    SendPacingMsg(ack_msg);
}