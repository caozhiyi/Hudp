#include "Log.h"
#include "IMsg.h"
#include "ISocket.h"
#include "SendWnd.h"
#include "HudpConfig.h"
#include "CommonFunc.h"
#include "IncrementalId.h"
#include "IPriorityQueue.h"

using namespace hudp;

CSendWndImpl::CSendWndImpl(uint16_t send_wnd_size, CPriorityQueue* priority_queue, bool always_send) :
                                                     _start(nullptr),
                                                     _end(nullptr),
                                                     _send_wnd_size(send_wnd_size),
                                                     _cur_send_size(0),
                                                     _always_send(always_send),
                                                     _priority_queue(priority_queue),
                                                     _out_of_order_count(0) {
    _incremental_id = new CIncrementalId(GetRandomInitialValue());
}

CSendWndImpl::~CSendWndImpl() {
    Clear();
}

void CSendWndImpl::PushBack(std::shared_ptr<CMsg> msg) {
    if (msg == nullptr) {
        base::LOG_WARN("send a nullptr to send wnd.");
        return;
    }

    if (!(msg->GetFlag() & msg_with_out_id)) {
        base::LOG_WARN("send a repeat mag to send wnd. id : %d", msg->GetId());
        return;
    }
    // can send now, push to priority queue.
    if (_cur_send_size >= _send_wnd_size) {
        _priority_queue->PushBack(msg);

    } else {
        PushBackToSendWnd(msg);
    }
}
// receive a ack
void CSendWndImpl::AcceptAck(uint16_t id) {

    base::LOG_DEBUG("send wnd recv a ack. id : %d", id);
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _id_msg_map.find(id);
    if (iter == _id_msg_map.end()) {
        return;
    }

    if (iter->second == _start) {
        // only one node
        if (_start == _end) {
            _end = nullptr;
        }
        _start = _start->GetNext();
        if (_start) {
            _start->SetPrev(nullptr);
        }
        _out_of_order_count = 0;

    } else if (iter->second == _end) {
        _end = _end->GetPrev();
        if (_end) {
            _end->SetNext(nullptr);
        }
        _out_of_order_count++;

    } else {
        _out_of_order_count++;
        Remove(iter->second);
    }

    if (_out_of_order_count >= __quick_resend_limit && _start) {
        _send_queue.push(_start);
        _out_of_order_count = 0;
    }

    _ack_queue.push(iter->second);
    _cur_send_size--;
    // send next bag
    SendNext();

    _id_msg_map.erase(iter);
 
    SendAndAck();
}

void CSendWndImpl::AcceptAck(uint16_t start_id, uint16_t len) {
    for (uint16_t index = start_id, i = 0; i < len; index++, i++) {
        AcceptAck(index);
    }
}

void CSendWndImpl::AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len) {
    len = start_index + len;
    for (; start_index < len; start_index++) {
        AcceptAck(vec_id[start_index]);
    }
}

// change send window size
void CSendWndImpl::ChangeSendWndSize(uint16_t size) {
    if (_send_wnd_size == size) {
        return;
    }

    _send_wnd_size = size;

    SendNext();

    SendAndAck();
}

// remove all msg
void CSendWndImpl::Clear() {
    for (auto iter = _id_msg_map.begin(); iter != _id_msg_map.end(); ++iter) {
        auto sock = iter->second->GetSocket();
        if (sock) {
            sock->AckDone(iter->second);
        }
    }
    _id_msg_map.clear();
}

void CSendWndImpl::SendAndAck() {
    std::shared_ptr<CMsg> item;

    while (!_send_queue.empty()) {
        item = _send_queue.front();
        _send_queue.pop();
        auto sock = item->GetSocket();
        sock->ToSend(item);
    }

    while (!_ack_queue.empty()) {
        item = _ack_queue.front();
        _ack_queue.pop();
        auto sock = item->GetSocket();
        sock->AckDone(item);
    }
}

void CSendWndImpl::SendNext() {
    // send next bag
    while (_cur_send_size < _send_wnd_size) {
        // add a msg to send wnd
        std::shared_ptr<CMsg> temp = _priority_queue->Pop();
        if (temp) {
            PushBackToSendWnd(temp);
            if (!_always_send) {
                _cur_send_size++;
            }

        } else {
            break;
        }
    } 
}

void CSendWndImpl::PushBackToSendWnd(std::shared_ptr<CMsg> msg) {
    uint16_t id = _incremental_id->GetNextId();
    msg->SetId(id);
    _id_msg_map[id] = msg;
   
    AddToEnd(msg);

    _send_queue.push(msg);
    if (!_always_send) {
        _cur_send_size++;
    }

    SendAndAck();
}

void CSendWndImpl::AddToEnd(std::shared_ptr<CMsg> msg) {
    // list is empty
    if (!_end) {
        _start = _end = msg;
        return;
    }

    _end->SetNext(msg);
    msg->SetPrev(_end);
    _end = msg;
}

void CSendWndImpl::Remove(std::shared_ptr<CMsg> msg) {
    if (!msg) {
        return;
    }
    if (msg == _start) {
        _start = _start->GetNext();
    }
    if (msg == _end) {
        _end = _end->GetPrev();
    }
    if (msg->GetPrev()) {
        msg->GetPrev()->SetNext(msg->GetNext());
    }
    if (msg->GetNext()) {
        msg->GetNext()->SetPrev(msg->GetPrev());
    }
}