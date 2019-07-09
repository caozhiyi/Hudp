#include <cstring>		//for memset

#include "Socket.h"
#include "SendWnd.h"
#include "OrderList.h"
#include "PriorityQueue.h"
#include "IncrementalId.h"
#include "NetMsg.h"
#include "FunctionNetMsg.h"
#include "Hudp.h"

using namespace hudp;

// this size may be a dynamic algorithm control
static const uint16_t __send_wnd_size = 5;

CSocket::CSocket() : _pri_queue(new CPriorityQueue) {
    memset(_inc_id, 0, sizeof(_inc_id));
    memset(_send_wnd, 0, sizeof(_send_wnd));
    memset(_recv_list, 0, sizeof(_recv_list));
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
    }
    delete _pri_queue;
}

NetMsg* CSocket::GetMsgFromPriQueue() {
    if (_pri_queue->Size()) {
        return _pri_queue->Pop();
    }
    return nullptr;
}

void CSocket::SendMsgToPriQueue(NetMsg* msg) {
    _pri_queue->Push(msg);
}

void CSocket::SendMsgToSendWnd(NetMsg* msg) {
    if (msg->_head._flag & HTF_ORDERLY) {
        if (_send_wnd[WI_ORDERLY]) {
            msg->_head._id = _inc_id[WI_ORDERLY]->GetNextId();
            _send_wnd[WI_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderOrderlyNetMsg*>(msg));
        
        } else {
            CreateSendWnd(WI_ORDERLY);
            msg->_head._id = _inc_id[WI_ORDERLY]->GetNextId();
            _send_wnd[WI_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderOrderlyNetMsg*>(msg));
        }
    
    } else if (msg->_head._flag & HTF_RELIABLE) {
        if (_send_wnd[WI_RELIABLE]) {
            msg->_head._id = _inc_id[WI_RELIABLE]->GetNextId();
            _send_wnd[WI_RELIABLE]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));

        } else {
            CreateSendWnd(WI_RELIABLE);
            msg->_head._id = _inc_id[WI_RELIABLE]->GetNextId();
            _send_wnd[WI_RELIABLE]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
        }

    } else if (msg->_head._flag & HTF_RELIABLE_ORDERLY) {
        if (_send_wnd[WI_RELIABLE_ORDERLY]) {
            msg->_head._id = _inc_id[WI_RELIABLE_ORDERLY]->GetNextId();
            _send_wnd[WI_RELIABLE_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));

        }
        else {
            CreateSendWnd(WI_RELIABLE_ORDERLY);
            msg->_head._id = _inc_id[WI_RELIABLE_ORDERLY]->GetNextId();
            _send_wnd[WI_RELIABLE_ORDERLY]->PushBack(msg->_head._id, dynamic_cast<CSenderRelialeOrderlyNetMsg*>(msg));
        }

    // normal udp msg. send to net
    } else {
        SendMsgToNet(msg);
    }
}

void CSocket::SendMsgToNet(NetMsg* msg) {
    CHudp::Instance().SendMsgToNet(msg);
}

void CSocket::RecvMsgUpper(NetMsg* msg) {
    CHudp::Instance().SendMsgToUpper(msg);
}

void CSocket::RecvMsgToOrderList(NetMsg* msg) {
    if (msg->_head._flag & HTF_ORDERLY) {
        if (_recv_list[WI_ORDERLY]) {
            _recv_list[WI_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));

        } else {
            CreateRecvList(WI_ORDERLY);
            _recv_list[WI_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));
        }

    } else if (msg->_head._flag & HTF_RELIABLE) {
        if (_recv_list[WI_RELIABLE]) {
            _recv_list[WI_RELIABLE]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));

        } else {
            CreateRecvList(WI_RELIABLE);
            _recv_list[WI_RELIABLE]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));
        }

    } else if (msg->_head._flag & HTF_RELIABLE_ORDERLY) {
        if (_recv_list[WI_RELIABLE_ORDERLY]) {
            _recv_list[WI_RELIABLE_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));

        } else {
            CreateRecvList(WI_RELIABLE_ORDERLY);
            _recv_list[WI_RELIABLE_ORDERLY]->Insert(msg->_head._id, dynamic_cast<CReceiverNetMsg*>(msg));
        }

    // normal udp msg. send to net
    } else {
        RecvMsgUpper(msg);
    }
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