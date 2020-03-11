#include "SendWnd.h"
#include "Log.h"
#include "IMsg.h"
#include "ISocket.h"
#include "IncrementalId.h"
#include "IPriorityQueue.h"

using namespace hudp;

CSendWndImpl::CSendWndImpl(uint16_t send_wnd_size, CPriorityQueue* priority_queue) : 
                                                     _end(nullptr),
                                                     _send_wnd_size(send_wnd_size),
                                                     _cur_send_size(0),
                                                     _priority_queue(priority_queue) {
    _start = _end;
    _cur = _end;
    _incremental_id = new CIncrementalId;
}

CSendWndImpl::~CSendWndImpl() {
    Clear();
}

void CSendWndImpl::PushBack(CMsg* msg) {
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
        _start = _start->GetNext();
        if (_start) {
            _start->SetPrev(nullptr);
        }

    } else if (iter->second == _end) {
        _end = _end->GetPrev();
        if (_end) {
            _end->SetNext(nullptr);
        }

    } else {
        // remove from list
        iter->second->GetNext()->SetPrev(iter->second->GetPrev());
        iter->second->GetPrev()->SetNext(iter->second->GetNext());
    }

    if (iter->second == _cur) {
        _cur = iter->second->GetNext();
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
    CMsg* item = nullptr;

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
    if (_cur_send_size < _send_wnd_size && _cur) {
        CMsg* temp = _priority_queue->Pop();    
        if (temp) {
            PushBackToSendWnd(temp);
        }

        _send_queue.push(_cur);
        _cur_send_size++;
        _cur = _cur->GetNext();
    } 
}

void CSendWndImpl::PushBackToSendWnd(CMsg* msg) {
    uint16_t id = _incremental_id->GetNextId();
    msg->SetId(id);
    _id_msg_map[id] = msg;
    msg->SetNext(nullptr);
    msg->SetPrev(nullptr);

    if (_end) {
        _end->SetNext(msg);
        msg->SetPrev(_end);
        _end = msg;
    }

    if (_cur == nullptr) {
        _cur = _end;
    }

    if (_start == nullptr) {
        _start = msg;
        _end = msg;
    }

    if (_cur_send_size < _send_wnd_size) {
        _send_queue.push(msg);
        _cur_send_size++;
        _cur = nullptr;
    }

    SendAndAck();
}