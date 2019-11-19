#include <cstring>		//for memset

#include "Socket.h"
#include "SendWnd.h"
#include "OrderList.h"
#include "PriorityQueue.h"
#include "IncrementalId.h"
#include "IMsg.h"
#include "HudpImpl.h"
#include "Timer.h"
#include "PendAck.h"
#include "Log.h"
#include "HudpConfig.h"
#include "CommonFlag.h"

using namespace hudp;

// this size may be a dynamic algorithm control
static const uint16_t __send_wnd_size = 5;

CSocketImpl::CSocketImpl(const Handle& handle) : _pri_queue(new CPriorityQueue), 
                                                 _handle(handle) {
    memset(_incremental_id, 0, sizeof(_incremental_id));
    memset(_send_wnd, 0, sizeof(_send_wnd));
    memset(_recv_list, 0, sizeof(_recv_list));
    memset(_pend_ack, 0, sizeof(_pend_ack));
}

CSocketImpl::~CSocketImpl() {
    for (uint16_t i = 0; i < __wnd_size; i++) {
        if (_incremental_id[i]) {
            delete _incremental_id[i];
        }
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
    delete _pri_queue;
}

Handle CSocketImpl::GetHandle() {
    return _handle;
}

void CSocketImpl::AddAckToMsg(CMsg* msg) {
    // clear prv ack info
    msg->ClearAck();

    if (_pend_ack[WI_RELIABLE_ORDERLY] && _pend_ack[WI_RELIABLE_ORDERLY]->HasAck()) {
        bool continuity = false;
        std::vector<uint16_t> ack_vec;
        // get acl from pend ack
        // TODO
        msg->SetAck(HPF_RELIABLE_ORDERLY_ACK_RANGE, ack_vec, continuity);
    }

    if (_pend_ack[WI_RELIABLE] && _pend_ack[WI_RELIABLE]->HasAck()) {
        bool continuity = false;
        std::vector<uint16_t> ack_vec;
        // get acl from pend ack
        // TODO
        msg->SetAck(HPF_WITH_RELIABLE_ACK, ack_vec, continuity);
    }
}

void CSocketImpl::GetAckToSendWnd(CMsg* msg) {
    if (msg->GetHeaderFlag() & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        auto time_stap = CTimer::Instance().GetTimeStamp();
        std::vector<uint16_t> vec;
        msg->GetAck(HPF_WITH_RELIABLE_ORDERLY_ACK, vec);
        for (uint16_t index = vec[0], i = 0; i < vec.size(); index++, i++) {
            _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(index);
            // TODO
            // set rtt sample
            //_rto.SetAckTime(index, time_stap);
        }
    }

    if (msg->GetHeaderFlag() & HPF_WITH_RELIABLE_ACK) {
        auto time_stap = CTimer::Instance().GetTimeStamp();
        std::vector<uint16_t> vec;
        msg->GetAck(HPF_WITH_RELIABLE_ACK, vec);
        for (uint16_t index = vec[0], i = 0; i < vec.size(); index++, i++) {
            _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(index);
            // TODO
            // set rtt sample
            //_rto.SetAckTime(index, time_stap);
        }
    }
}

void CSocketImpl::SendMessage(CMsg* msg) {
    auto header_flag = msg->GetHeaderFlag();
    if (header_flag & HTF_ORDERLY) {
        if (_send_wnd[WI_ORDERLY]->CanSendNow()) {
            _send_wnd[WI_ORDERLY]->PushBack(msg);
        } else {
            _pri_queue->Push(msg);
        }

    } else if (header_flag & HTF_RELIABLE) {
        if (_send_wnd[WI_RELIABLE]->CanSendNow()) {
            _send_wnd[WI_RELIABLE]->PushBack(msg);
        } else {
            _pri_queue->Push(msg);
        }

    } else if (header_flag & HTF_RELIABLE_ORDERLY) {
        if (_send_wnd[WI_RELIABLE_ORDERLY]->CanSendNow()) {
            _send_wnd[WI_RELIABLE_ORDERLY]->PushBack(msg);
        } else {
            _pri_queue->Push(msg);
        }

    } else{
        CHudpImpl::Instance().SendMessageToNet(msg);
    }
}

void CSocketImpl::RecvMessage(CMsg* msg) {
    // get ack info
    GetAckToSendWnd(msg);
    // recv msg to orderlist
    bool done = false;
    uint16_t ret = 0;

    auto flag = msg->GetHeaderFlag();
    // reliable and orderly
    if (flag & HTF_RELIABLE_ORDERLY) {
        if (_recv_list[WI_RELIABLE_ORDERLY]) {
            ret = _recv_list[WI_RELIABLE_ORDERLY]->Insert(msg);

        } else {
            CreateRecvList(WI_RELIABLE_ORDERLY);
            ret = _recv_list[WI_RELIABLE_ORDERLY]->Insert(msg);
        }
        done = true;

    // only orderly
    } else if (flag & HTF_ORDERLY) {
        if (_recv_list[WI_ORDERLY]) {
            ret = _recv_list[WI_ORDERLY]->Insert(msg);

        } else {
            CreateRecvList(WI_ORDERLY);
            ret = _recv_list[WI_ORDERLY]->Insert(msg);
        }
        done = true;

    // only reliable
    } else if (flag & HTF_RELIABLE) {
        if (_recv_list[WI_RELIABLE]) {
            ret = _recv_list[WI_RELIABLE]->Insert(msg);

        } else {
            CreateRecvList(WI_RELIABLE);
            ret = _recv_list[WI_RELIABLE]->Insert(msg);
        }
        done = true;
    }

    // get a repeat msg
    if (ret == 1) {
        AddAck(msg);
        return;
    }

    // normal udp. 
    if (!done) {
        CHudpImpl::Instance().RecvMessageToUpper(_handle, msg);
    }
}

void CSocketImpl::ToRecv(CMsg* msg) {
    // send ack msg to remote
    base::LOG_DEBUG("[receiver] :receiver msg. id : %d", msg->GetId());
    AddAck(msg);
    CHudpImpl::Instance().RecvMessageToUpper(_handle, msg);
}

void CSocketImpl::ToSend(CMsg* msg, CSendWnd* send_wnd) {
    // add to timer
    if (msg->GetHeaderFlag() & HPF_NEED_ACK) {
        // TODO
    }

    CHudpImpl::Instance().SendMessageToNet(msg);

    
    send_wnd->PushBack();
}

void CSocketImpl::AckDone(CMsg* msg) {
    // release msg here
    CHudpImpl::Instance().ReleaseMessage(msg);
}

void CSocketImpl::TimerOut(CMsg* msg) {
    if (!(msg->GetFlag() & msg_is_only_ack)) {
        msg->AddSendDelay();
    }

    // add ack 
    AddAckToMsg(msg);
    // send to net
    CHudpImpl::Instance().SendMessageToNet(msg);
}

void CSocket::RecvAck(NetMsg* msg) {
    if (msg->_head._flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        if (msg->_head._flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {

            auto time_stap = CTimer::Instance().GetTimeStamp();
            for (uint16_t index = msg->_head._ack_vec[0], i = 0; i < msg->_head._ack_reliable_orderly_len; index++, i++) {
                _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(index);
                // set rtt sample
                _rto.SetAckTime(index, time_stap);
            }

        } else {

            auto time_stap = CTimer::Instance().GetTimeStamp();
            uint16_t start_index = 0;
            for (; start_index < msg->_head._ack_reliable_orderly_len; start_index++) {
                _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(msg->_head._ack_vec[start_index]);
                // set rtt sample
                _rto.SetAckTime(msg->_head._ack_vec[start_index], time_stap);
            }
        }
    } 

    if (msg->_head._flag & HPF_WITH_RELIABLE_ACK) {
        if (msg->_head._flag & HPF_RELIABLE_ACK_RANGE) {

            auto time_stap = CTimer::Instance().GetTimeStamp();
            for (uint16_t index = msg->_head._ack_vec[msg->_head._ack_reliable_orderly_len], i = 0; i < msg->_head._ack_reliable_len; index++, i++) {
                _send_wnd[WI_RELIABLE]->AcceptAck(index);
                // set rtt sample
                _rto.SetAckTime(index, time_stap);
            }

        } else {

            auto time_stap = CTimer::Instance().GetTimeStamp();
            uint16_t start_index = msg->_head._ack_reliable_orderly_len;
            for (; start_index < msg->_head._ack_reliable_len; start_index++) {
                _send_wnd[WI_RELIABLE]->AcceptAck(msg->_head._ack_vec[start_index]);
                // set rtt sample
                _rto.SetAckTime(msg->_head._ack_vec[start_index], time_stap);
            }
        }
    }
}

void CSocket::RecvMsgUpper(NetMsg* msg) {
    CHudpImpl::Instance().SendMsgToUpper(msg);
}

void CSocket::RecvMsgToOrderList(NetMsg* msg) {
    bool done = false;
    uint16_t ret = 0;
    // reliable and orderly
    if (msg->_head._flag & HPF_IS_ORDERLY && msg->_head._flag & HPF_NEED_ACK) {
        if (_recv_list[WI_RELIABLE_ORDERLY]) {
            ret = _recv_list[WI_RELIABLE_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));

        } else {
            CreateRecvList(WI_RELIABLE_ORDERLY);
            ret = _recv_list[WI_RELIABLE_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));
        }
        done = true;

    // only orderly
    } else if (msg->_head._flag & HPF_IS_ORDERLY) {
        if (_recv_list[WI_ORDERLY]) {
            ret = _recv_list[WI_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));

        } else {
            CreateRecvList(WI_ORDERLY);
            ret = _recv_list[WI_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));
        }
        done = true;

    // only reliable
    } else if (msg->_head._flag & HPF_NEED_ACK) {
        if (_recv_list[WI_RELIABLE]) {
            ret = _recv_list[WI_RELIABLE]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));

        } else {
            CreateRecvList(WI_RELIABLE);
            ret = _recv_list[WI_RELIABLE]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));
        }
        done = true;
    }

    // get a repeat msg
    if (ret == 1) {
        AddAck(msg);
        return;
    }

    // normal udp. 
    if (!done) {
         RecvMsgUpper(msg);
    }
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
        // add to timer
        // TODO
        _is_in_timer = true;
    }
}

uint64_t CSocket::AddToTimer(CSenderRelialeOrderlyNetMsg* msg) {
    uint32_t time = _rto.GetRto() * msg->_backoff_factor;
    time = time > __max_rto_time ? __max_rto_time : time;
    base::LOG_DEBUG("[resend] time : %d", time);
    return msg->Attach(time);
}

void CSocketImpl::AddToSendWnd(WndIndex index, CMsg* msg) {
    if (!_send_wnd[index]) {
        _send_wnd[index] = new CSendWnd();
    }
    _send_wnd[index]->PushBack(msg);
}

void CSocketImpl::AddToRecvList(WndIndex index, CMsg* msg) {
    if (!_recv_list[index]) {
        if (index == WI_ORDERLY) {
            _recv_list[index] = new COrderlyList();

        } else if (index == WI_RELIABLE) {
            _recv_list[index] = new CReliableList();

        } else if (index == WI_RELIABLE_ORDERLY) {
            _recv_list[index] = new CReliableOrderlyList();
        }
    }
    _recv_list[index]->Insert(msg);
}

void CSocketImpl::AddToPendAck(WndIndex index, CMsg* msg) {
    if (!_pend_ack[index]) {
        _pend_ack[index] = new CPendAck();
    }
    _pend_ack[index]->AddAck(msg->GetId());
}

void CSocketImpl::AddToPriorityQueue(WndIndex index, CMsg* msg) {
    if (!_pri_queue[index]) {
        _pri_queue[index] = new CPriorityQueue();
    }
    _pri_queue[index]->PushBack(msg);
}