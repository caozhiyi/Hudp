#include "SendWnd.h"
#include "Log.h"
#include "IMsg.h"
#include "ISocket.h"

using namespace hudp;

CSendWndImpl::CSendWndImpl(uint16_t send_wnd_size) : _end(nullptr),
                                             _send_wnd_size(send_wnd_size),
                                             _cur_send_size(0) {
    _start = _end;
    _cur = _end;
}

CSendWndImpl::~CSendWndImpl() {
    Clear();
}

void CSendWndImpl::PushBack(uint16_t id, CMsg* msg) {
    if (msg == nullptr) {
        base::LOG_WARN("send a nullptr to send wnd. id : %d", id);
        return;
    }

    // repeat msg
    if (_id_map.find(id) != _id_map.end()) {
        base::LOG_WARN("send a repeat mag to send wnd. id : %d", id);
        return;
    }

    _id_map[id] = msg;
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
        _send_queue.push_back(msg);
        _cur_send_size++;
        _cur = nullptr;
    }

    SendAndAck();
}
// receive a ack
void CSendWndImpl::AcceptAck(uint16_t id) {

    base::LOG_DEBUG("send wnd recv a ack. id : %d", id);
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _id_map.find(id);
    if (iter == _id_map.end()) {
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

    _ack_queue.push_back(iter->second);
    _cur_send_size--;
    // send next bag
    SendNext();

    _id_map.erase(iter);
 
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
    for (auto iter = _id_map.begin(); iter != _id_map.end(); ++iter) {
        auto sock = iter->second->GetSocket();
        if (sock) {
            sock->AckDone(iter->second);
        }
    }
    _id_map.clear();
}


void CSendWndImpl::SetIndexResend(uint16_t id) {
    CMsg* msg = nullptr;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto iter = _id_map.find(id);
        if (iter == _id_map.end()) {
            return;
        }
        msg = iter->second;
    }
    
    if (msg) {
        auto sock = msg->GetSocket();
        if (sock) {
            sock->ToSend(msg);
        }
    }
}


void CSendWndImpl::SendAndAck() {
    /*CMsg* item = nullptr;

    while (_send_queue.Pop(item)) {
        item->ToSend();
    }
    _send_queue.Clear();
    while (_ack_queue.Pop(item)) {
        item->AckDone();   
    }
    _ack_queue.Clear();*/
}

void CSendWndImpl::SendNext() {
    // send next bag
    if (_cur_send_size < _send_wnd_size && _cur) {
        _send_queue.push_back(_cur);
        _cur_send_size++;
        _cur = _cur->GetNext();
    
    } 
}
