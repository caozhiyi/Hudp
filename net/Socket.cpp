#include <cstring>		//for memset

#include "Socket.h"
#include "SendWnd.h"
#include "OrderList.h"
#include "PriorityQueue.h"
#include "IncrementalId.h"
#include "NetMsg.h"
#include "FunctionNetMsg.h"
#include "HudpImpl.h"
#include "Timer.h"
#include "PendAck.h"
#include "NetMsgPool.h"
#include "BitStreamPool.h"
#include "SocketManager.h"
#include "Log.h"

using namespace hudp;

// this size may be a dynamic algorithm control
static const uint16_t __send_wnd_size = 5;

CSocket::CSocket(const HudpHandle& handle) : _handle(handle), _pri_queue(new CPriorityQueue) {
    memset(_inc_id, 0, sizeof(_inc_id));
    memset(_send_wnd, 0, sizeof(_send_wnd));
    memset(_recv_list, 0, sizeof(_recv_list));
    memset(_pend_ack, 0, sizeof(_pend_ack));
    // init out timer
    _timer_out_time = 4000;
    _is_in_timer = false;
}

CSocket::~CSocket() {
    for (uint16_t i = 0; i < __wnd_size; i++) {
        if (_inc_id[i]) {
            delete _inc_id[i];
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

NetMsg* CSocket::GetMsgFromPriQueue() {
    if (_pri_queue->Size()) {
        return _pri_queue->Pop();
    }
    base::LOG_WARN("socket return a null msg.");
    return nullptr;
}

void CSocket::SendMsgToPriQueue(NetMsg* msg) {
    _pri_queue->Push(msg);
    CSocketManager::Instance().NotifyMsg(_handle);
}

void CSocket::SendMsgToSendWnd(NetMsg* msg) {
    if (msg->_head._flag & HPF_NEED_ACK && msg->_head._flag & HPF_IS_ORDERLY) {
        if (_send_wnd[WI_RELIABLE_ORDERLY]) {
            msg->SetId(_inc_id[WI_RELIABLE_ORDERLY]->GetNextId());
            _send_wnd[WI_RELIABLE_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
            AddToTimer(dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));

        } else {
            CreateSendWnd(WI_RELIABLE_ORDERLY);
            msg->SetId(_inc_id[WI_RELIABLE_ORDERLY]->GetNextId());
            _send_wnd[WI_RELIABLE_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
            AddToTimer(dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
        }

    } else if (msg->_head._flag & HPF_IS_ORDERLY) {
        if (_send_wnd[WI_ORDERLY]) {
            msg->SetId(_inc_id[WI_ORDERLY]->GetNextId());
            _send_wnd[WI_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderOrderlyNetMsg*>(msg));
        
        } else {
            CreateSendWnd(WI_ORDERLY);
            msg->SetId(_inc_id[WI_ORDERLY]->GetNextId());
            _send_wnd[WI_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderOrderlyNetMsg*>(msg));
        }
    
    } else if (msg->_head._flag & HPF_NEED_ACK) {
        if (_send_wnd[WI_RELIABLE]) {
            msg->SetId(_inc_id[WI_RELIABLE]->GetNextId());
            _send_wnd[WI_RELIABLE]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
            AddToTimer(dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));

        } else {
            CreateSendWnd(WI_RELIABLE);
            msg->SetId(_inc_id[WI_RELIABLE]->GetNextId());
            _send_wnd[WI_RELIABLE]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
            AddToTimer(dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
        }

    // normal udp msg. send to net
    } else {
        msg->_phase += 1;
    }
}

void CSocket::SendMsgToNet(NetMsg* msg) {
    // already with ack
    if (msg->_head._flag & HPF_WITH_ACK_ARRAY || msg->_head._flag & HPF_WITH_ACK_RANGE) {
        CHudpImpl::Instance().SendMsgToNet(msg);
        return;
    }
    
    // add ack to net msg
    if (msg->_head._flag & HPF_NEED_ACK && msg->_head._flag & HPF_IS_ORDERLY) {
        if (_pend_ack[WI_RELIABLE_ORDERLY]->GetConsecutiveAck(msg->_head._ack_start, msg->_head._ack_len)) {
            msg->_head._flag |= HPF_WITH_ACK_RANGE;
        }

    } else if (msg->_head._flag & HPF_NEED_ACK) {
        if (_pend_ack[WI_RELIABLE]->GetAllAck(msg->_head._ack_vec)) {
            msg->_head._flag |= HPF_WITH_ACK_ARRAY;
        }
    }
}

void CSocket::RecvAck(NetMsg* msg) {
    if (msg->_head._flag & HPF_WITH_ACK_ARRAY) {
        _send_wnd[WI_RELIABLE]->AcceptAck(msg->_head._ack_vec);

    } else if (msg->_head._flag & HPF_WITH_ACK_RANGE) {
        _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(msg->_head._ack_start, msg->_head._ack_len);
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
        RecvAck(msg);
        return;
    }

    // with ack
    if (msg->_head._flag & HPF_WITH_ACK_RANGE) {
        _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(msg->_head._ack_start, msg->_head._ack_len);
        // the msg only with ack. release here
        if (!done && msg->_head._body_len == 0) {
           CNetMsgPool::Instance().FreeMsg(msg, true);
           done = true;
        }
        
    } else if (msg->_head._flag & HPF_WITH_ACK_ARRAY) {
        for (auto iter = msg->_head._ack_vec.begin(); iter != msg->_head._ack_vec.end(); ++iter) {
            _send_wnd[WI_RELIABLE_ORDERLY]->AcceptAck(*iter);
        }
        // the msg only with ack. release here
        if (!done && msg->_head._body_len == 0) {
           CNetMsgPool::Instance().FreeMsg(msg, true);
           done = true;
        }
    }

    // normal udp. 
    if (!done) {
         RecvMsgUpper(msg);
    }
}

void CSocket::AddAck(NetMsg* msg) {
    if (msg->_head._flag & HPF_IS_ORDERLY && msg->_head._flag & HPF_NEED_ACK) {
        if (_pend_ack[WI_RELIABLE_ORDERLY]) {
            _pend_ack[WI_RELIABLE_ORDERLY]->AddAck(msg->_head._id);

        } else {
            CreatePendAck(WI_RELIABLE_ORDERLY);
            _pend_ack[WI_RELIABLE_ORDERLY]->AddAck(msg->_head._id);
        }

    } else if (msg->_head._flag & HPF_IS_ORDERLY) {
        if (_pend_ack[WI_RELIABLE]) {
            _pend_ack[WI_RELIABLE]->AddAck(msg->_head._id);

        } else {
            CreatePendAck(WI_RELIABLE);
            _pend_ack[WI_RELIABLE]->AddAck(msg->_head._id);
        }

    }

    // add to timer
    if (!_is_in_timer) {
        CTimer::Instance().AddTimer(_timer_out_time.load(), this);
        _is_in_timer = true;
    }
}

void CSocket::OnTimer() {
    // create a ack msg and send to remote.
    if (_pend_ack[WI_RELIABLE]) {
        std::vector<uint16_t> ack_vec;
        // ack may added to net msg already, check pendack
        if (_pend_ack[WI_RELIABLE]->GetAllAck(ack_vec)) {
            NetMsg* msg = CNetMsgPool::Instance().GetAckMsg();
            msg->_head._flag |= HPF_WITH_ACK_ARRAY;
            msg->_head._ack_vec = std::move(ack_vec);
            msg->_ip_port = _handle;
            SendMsgToPriQueue(msg);
        }
       

    } else if (_pend_ack[WI_RELIABLE_ORDERLY]) {
        uint16_t start = 0;
        uint16_t len = 0;
        
        // ack may added to net msg already, check pendack 
        if (_pend_ack[WI_RELIABLE_ORDERLY]->GetConsecutiveAck(start, len)){
            NetMsg* msg = CNetMsgPool::Instance().GetAckMsg();
            msg->_head._flag |= HPF_WITH_ACK_RANGE;
            msg->_head._ack_start = start;
            msg->_head._ack_len = len;
            msg->_ip_port = _handle;
            SendMsgToPriQueue(msg);
        }
    }
    _is_in_timer = false;
}

void CSocket::SetTimerOutTime(uint16_t timer_out) {
    _timer_out_time = timer_out;
}

void CSocket::AddToTimer(CSenderRelialeOrderlyNetMsg* msg) {
    CTimer::Instance().AddTimer(_timer_out_time, msg);
}

void CSocket::CreateSendWnd(WndIndex index) {
    if (_send_wnd[index] == nullptr) {
        _send_wnd[index] = new CSendWnd(__send_wnd_size);
    }

    if (_inc_id[index] == nullptr) {
        _inc_id[index] = new CIncrementalId();
    }
}

void CSocket::CreateRecvList(WndIndex index) {
    if (_recv_list[index] == nullptr) {
        if (index == WI_ORDERLY) {
            _recv_list[index] = new COrderlyList();
        
        } else if (index == WI_RELIABLE) {
            _recv_list[index] = new CReliableList();
        
        } else if (index == WI_RELIABLE_ORDERLY) {
            _recv_list[index] = new CReliableOrderlyList();
        }
    }
}

void CSocket::CreatePendAck(WndIndex index) {
    if (_pend_ack[index] == nullptr) {
        _pend_ack[index] = new CPendAck();
    }
}