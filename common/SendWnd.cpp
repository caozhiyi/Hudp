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
    // push to priority queue.
    if (_cur_send_size >= _send_wnd_size) {
        _priority_queue->PushBack(msg);

    } else {
        // send to remote directly.
        PushBackToSendWnd(msg);
    }
}
// receive a ack
uint32_t CSendWndImpl::AcceptAck(uint16_t id) {
    base::LOG_DEBUG("send wnd recv a ack. id : %d", id);
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _id_msg_map.find(id);
    if (iter == _id_msg_map.end()) {
        // a repeat ack.
        return 0;
    }

    if (iter->second == _start) {
        _out_of_order_count = 0;

    } else {
        _out_of_order_count++;
    }

    _ack_queue.push(iter->second);
    _cur_send_size--;

    uint32_t size = iter->second->GetEstimateSize();
    _id_msg_map.erase(iter);

    if (!_id_msg_map.empty()) {
        _start = _id_msg_map.begin()->second;
    }
    else {
        _start.reset();
    }

    if (_out_of_order_count >= __quick_resend_limit && _start) {
        _send_queue.push(_start);
        _out_of_order_count = 0;
    }

    // send next bag
    SendNext();
    SendAndAck();
    return size;
}

uint32_t CSendWndImpl::AcceptAck(uint16_t start_id, uint16_t len) {
    uint32_t ret = 0;
    for (uint16_t index = start_id, i = 0; i < len; index++, i++) {
        ret += AcceptAck(index);
    }
    return ret;
}

uint32_t CSendWndImpl::AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len) {
    len = start_index + len;
    uint32_t ret = 0;
    for (; start_index < len; start_index++) {
        ret += AcceptAck(vec_id[start_index]);
    }
    return ret;
}

bool CSendWndImpl::IsAppLimit() {
    return _send_wnd_size == (uint16_t)_id_msg_map.size();
}

uint16_t CSendWndImpl::GetWndSize() {
    return _send_wnd_size;
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
    if (!msg) {
        return;
    }
    _id_msg_map[msg->GetId()] = msg;
    if (!_start) {
        _start = _id_msg_map.begin()->second;
    }
}

void CSendWndImpl::Remove(std::shared_ptr<CMsg> msg) {
    if (!msg) {
        return;
    }
    _id_msg_map.erase(msg->GetId());
}