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
#include "FunctionNetMsg.h"
#include "Serializes.h"

using namespace hudp;

// this size may be a dynamic algorithm control
static const uint16_t __send_wnd_size = 5;
static const uint16_t __pend_ack_send = 50;     // 50ms
static const uint16_t __max_rto_time  = 12000;  // max rto 120s 

CSocket::CSocket(const HudpHandle& handle) : _pri_queue(new CPriorityQueue), _handle(handle) {
    memset(_inc_id, 0, sizeof(_inc_id));
    memset(_send_wnd, 0, sizeof(_send_wnd));
    memset(_recv_list, 0, sizeof(_recv_list));
    memset(_pend_ack, 0, sizeof(_pend_ack));
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

        } else {
            CreateSendWnd(WI_RELIABLE_ORDERLY);
            msg->SetId(_inc_id[WI_RELIABLE_ORDERLY]->GetNextId());
            _send_wnd[WI_RELIABLE_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
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

        } else {
            CreateSendWnd(WI_RELIABLE);
            msg->SetId(_inc_id[WI_RELIABLE]->GetNextId());
            _send_wnd[WI_RELIABLE]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
        }

    // normal udp msg. send to net
    } else {
        msg->_phase += 1;
    }
}

void CSocket::SendMsgToNet(NetMsg* msg) {
    // add ack info to msg
    AttachPendAck(msg);

    // serializes
    Serializes(msg);
    
    // add to timer
    if (msg->_head._flag & HPF_NEED_ACK) {
        auto socket = msg->_socket.lock();
        if (socket) {
            uint64_t time_stamp = socket->AddToTimer(dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
            if (!msg->_re_send) {
                // set rtt sample
                _rto.SetIdTime(msg->_head._id, time_stamp);

            } else {
                _rto.RemoveIdTime(msg->_head._id);
            }
        } 
    }
    
    CHudpImpl::Instance().SendMsgToNet(msg);
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

void CSocket::AddAck(NetMsg* msg) {
    if (msg->_head._flag & HPF_IS_ORDERLY && msg->_head._flag & HPF_NEED_ACK) {
        if (_pend_ack[WI_RELIABLE_ORDERLY]) {
            _pend_ack[WI_RELIABLE_ORDERLY]->AddAck(msg->_head._id);

        } else {
            CreatePendAck(WI_RELIABLE_ORDERLY);
            _pend_ack[WI_RELIABLE_ORDERLY]->AddAck(msg->_head._id);
        }

    } else if (msg->_head._flag & HPF_NEED_ACK) {
        if (_pend_ack[WI_RELIABLE]) {
            _pend_ack[WI_RELIABLE]->AddAck(msg->_head._id);

        } else {
            CreatePendAck(WI_RELIABLE);
            _pend_ack[WI_RELIABLE]->AddAck(msg->_head._id);
        }
    }

    // add to timer
    if (!_is_in_timer) {
        Attach(__pend_ack_send);
        _is_in_timer = true;
    }
}

bool CSocket::AttachPendAck(NetMsg* msg) {
    // clear prv ack info
    msg->ClearAck();

    if (_pend_ack[WI_RELIABLE_ORDERLY] && _pend_ack[WI_RELIABLE_ORDERLY]->HasAck()) {
        bool continuity = false;
        bool ret = _pend_ack[WI_RELIABLE_ORDERLY]->GetAllAck(msg->_head._ack_vec, continuity);
        if (ret && continuity) {
            msg->_head._flag |= HPF_RELIABLE_ORDERLY_ACK_RANGE;
        }
        msg->_head._ack_reliable_orderly_len = msg->_head._ack_vec.size();
        msg->_head._flag |= HPF_WITH_RELIABLE_ORDERLY_ACK;
        msg->_flag = true;
    }

    if (_pend_ack[WI_RELIABLE] && _pend_ack[WI_RELIABLE]->HasAck()) {
        bool continuity = false;
        bool ret = _pend_ack[WI_RELIABLE]->GetAllAck(msg->_head._ack_vec, continuity);
        if (ret && continuity) {
            msg->_head._flag |= HPF_RELIABLE_ACK_RANGE;
        }
        msg->_head._ack_reliable_len = msg->_head._ack_vec.size() - msg->_head._ack_reliable_orderly_len;
        msg->_head._flag |= HPF_WITH_RELIABLE_ACK;
        msg->_flag = true;
    }
    return msg->_flag;
}

void CSocket::OnTimer() {
    // create a ack msg and send to remote.
    if ((_pend_ack[WI_RELIABLE] && _pend_ack[WI_RELIABLE]->HasAck())
       || (_pend_ack[WI_RELIABLE_ORDERLY] && _pend_ack[WI_RELIABLE_ORDERLY]->HasAck())) {
        // ack may added to net msg already, check pendack
        NetMsg* msg = CNetMsgPool::Instance().GetNormalMsg();
        msg->_socket = shared_from_this();
        msg->_ip_port = _handle;
        SendMsgToNet(msg);
    }

    _is_in_timer = false;
}

uint64_t CSocket::AddToTimer(CSenderRelialeOrderlyNetMsg* msg) {
    uint32_t time = _rto.GetRto() * msg->_backoff_factor;
    time = time > __max_rto_time ? __max_rto_time : time;
    base::LOG_DEBUG("[resend] time : %d", time);
    return msg->Attach(time);
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

bool CSocket::Serializes(NetMsg* msg) {
    if (msg->_bit_stream) {
        if (msg->_flag) {
            msg->_bit_stream->Clear();
            CBitStreamWriter* temp_bit_stream = static_cast<CBitStreamWriter*>(msg->_bit_stream);
            if (CSerializes::Serializes(*msg, *temp_bit_stream)) {
                msg->_bit_stream = temp_bit_stream;
                return true;            

            } else {
                base::LOG_ERROR("serializes msg to stream failed. id : %d, handle : %s", msg->_head._id, msg->_ip_port.c_str());
                return false;
            }
        } 

    } else {
        CBitStreamWriter* temp_bit_stream = static_cast<CBitStreamWriter*>(CBitStreamPool::Instance().GetBitStream());

        if (CSerializes::Serializes(*msg, *temp_bit_stream)) {
            msg->_bit_stream = temp_bit_stream;

        } else {
            base::LOG_ERROR("serializes msg to stream failed. id : %d, handle : %s", msg->_head._id, msg->_ip_port.c_str());
            return false;
        }
    }
   
    return true;
}

bool CSocket::Deseriali(NetMsg* msg) {
    CBitStreamReader* temp_bit_stream = static_cast<CBitStreamReader*>(msg->_bit_stream);
    msg->_socket = shared_from_this();
    if (!CSerializes::Deseriali(*temp_bit_stream, *msg)) {
        base::LOG_ERROR("deserialize stream to msg failed. id : %d, handle : %s", msg->_head._id, msg->_ip_port.c_str());
        return false;
    }
    return true;
}

HudpHandle CSocket::GetHandle() {
    return _handle;
}
