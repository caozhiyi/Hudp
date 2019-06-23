#include "SendWnd.h"
using namespace hudp;

CSendWnd::CSendWnd(uint16_t send_wnd_size) : _end(nullptr),
                                             _cur_send_size(0),
                                             _send_wnd_size(send_wnd_size) {
    _start = _end;
    _cur = _end;
}

CSendWnd::~CSendWnd() {

}

void CSendWnd::PushBack(uint16_t id, CSendWndSolt* data) {
    if (data == nullptr) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(_mutex);
        _id_map[id] = data;
        if (_end) {
            _end->_next = data;
            _end = data;

        } else {
            _end = data;
        }

        if (_cur_send_size < _send_wnd_size) {
            _send_queue.Push(data);
            _cur_send_size++;
            _cur = _end;
        }
    }
    
    SendAndAck();
}

void CSendWnd::AcceptAck(uint16_t id) {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto iter = _id_map.find(id);
        if (iter == _id_map.end()) {
            return;
        }

        iter->second->_done_ack = true;
        // call back done ack
        if (iter->second == _start) {
            while (_start->_done_ack && _start) {
                // move start point
                _ack_queue.Push(_start);
                _start = _start->_next;
                _cur_send_size--;

                // send next bag
                if (_cur_send_size < _send_wnd_size && (_cur->_next || _cur)) {
                    _send_queue.Push(_cur);
                    _cur_send_size++;
                    if (_cur->_next) {
                        _cur = _cur->_next;
                    }
                }
            }
        }
        _id_map.erase(iter);
    }
    SendAndAck();
}

void CSendWnd::AcceptAck(uint16_t start_id, uint16_t len) {
    for (uint16_t i = start_id; i < len; i++) {
        AcceptAck(i);
    }
}

void CSendWnd::ChangeSendWndSize(uint16_t size) {
    if (_send_wnd_size == size) {
        return;
    }

    std::unique_lock<std::mutex> lock(_mutex);
    _send_wnd_size = size;
}

CSendWndSolt* CSendWnd::GetIndexData(uint16_t id) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _id_map.find(id);
    if (iter == _id_map.end()) {
        return nullptr;
    }
    return iter->second;
}

void CSendWnd::SetIndexResend(uint16_t id) {
    CSendWndSolt* solt = nullptr;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto iter = _id_map.find(id);
        if (iter == _id_map.end()) {
            return;
        }
        solt = iter->second;
    }
    
    if (solt) {
        solt->ToSend();
    }
}

void CSendWnd::SendAndAck() {
    CSendWndSolt* item = nullptr;

    while (_send_queue.Pop(item)) {
        item->ToSend();
    }
    _send_queue.Clear();
    while (_ack_queue.Pop(item)) {
        item->AckDone();   
    }
    _ack_queue.Clear();
}