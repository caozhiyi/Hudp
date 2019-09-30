#include "SendWnd.h"
#include "Log.h"
using namespace hudp;

CSendWnd::CSendWnd(uint16_t send_wnd_size) : _end(nullptr),
                                             _send_wnd_size(send_wnd_size),
                                             _cur_send_size(0) {
    _start = _end;
    _cur = _end;
}

CSendWnd::~CSendWnd() {
    Clear();
}

void CSendWnd::PushBack(uint16_t id, CSendWndSolt* data) {
    if (data == nullptr) {
        base::LOG_WARN("send a nullptr to send wnd. id : %d", id);
        return;
    }

    {
        std::unique_lock<std::mutex> lock(_mutex);
        // repeat msg
        if (_id_map.find(id) != _id_map.end()) {
            base::LOG_WARN("send a repeat mag to send wnd. id : %d", id);
            return;
        }
        
        _id_map[id] = data;
        data->_next = nullptr;
        data->_prev = nullptr;

        if (_end) {
            _end->_next = data;
            data->_prev = _end;
            _end = data;
        }

        if (_cur == nullptr) {
            _cur = _end;
        }

        if (_start == nullptr) {
            _start = data;
            _end = data;
        }

        if (_cur_send_size < _send_wnd_size) {
            _send_queue.Push(data);
            _cur_send_size++;
            _cur = nullptr;
        }
    }
    SendAndAck();
}

void CSendWnd::AcceptAck(uint16_t id) {
    {
        base::LOG_DEBUG("send wnd recv a ack. id : %d", id);
        std::unique_lock<std::mutex> lock(_mutex);
        auto iter = _id_map.find(id);
        if (iter == _id_map.end()) {
            return;
        }

        if (iter->second == _start) {
            _start = _start->_next;
            if (_start) {
                _start->_prev = nullptr;
            }

        } else if (iter->second == _end) {
            _end = _end->_prev;
            if (_end) {
                _end->_next = nullptr;
            }

        } else {
            // remove fron list
            iter->second->_next->_prev = iter->second->_prev;
            iter->second->_prev->_next = iter->second->_next;
        }

        if (iter->second == _cur) {
            _cur = iter->second->_next;
        } 

        _ack_queue.Push(iter->second);
        _cur_send_size--;
        // send next bag
        SendNext();

        _id_map.erase(iter);
    }
    SendAndAck();
}

void CSendWnd::AcceptAck(uint16_t start_id, uint16_t len) {
    for (uint16_t index = start_id, i = 0; i < len; index++, i++) {
        AcceptAck(index);
    }
}

void CSendWnd::AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len) {
    len = start_index + len;
    for (; start_index < len; start_index++) {
        AcceptAck(vec_id[start_index]);
    }
}

void CSendWnd::ChangeSendWndSize(uint16_t size) {
    if (_send_wnd_size == size) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(_mutex);
        _send_wnd_size = size;

        SendNext();
    }

    SendAndAck();
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

void CSendWnd::Clear() {
    std::unique_lock<std::mutex> lock(_mutex);
    for (auto iter = _id_map.begin(); iter != _id_map.end(); ++iter) {
        iter->second->AckDone();
    }
    _id_map.clear();
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

void CSendWnd::SendNext() {
    // send next bag
    if (_cur_send_size < _send_wnd_size && _cur) {
        _send_queue.Push(_cur);
        _cur_send_size++;
        _cur = _cur->_next;
    
    } 
}
