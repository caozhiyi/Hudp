#include "PriorityQueue.h"
#include "CommonFlag.h"
#include "Log.h"
using namespace hudp;

CPriorityQueue::CPriorityQueue() : _pri_normal_count(__pri_surplus),
                                   _pri_heig_count(__pri_surplus),
                                   _pri_heighest_count(__pri_surplus),
                                   _size(0) {

    _cur_queue = &_queue_arr[__pri_heighest];
}

CPriorityQueue::~CPriorityQueue() {

}

void CPriorityQueue::Push(NetMsg* msg) {
    if (msg->_head._flag & HPF_LOW_PRI) {
        _queue_arr[__pri_low].Push(msg);

    } else if (msg->_head._flag & HPF_NROMAL_PRI) {
        _queue_arr[__pri_normal].Push(msg);

    } else if (msg->_head._flag & HPF_HIGH_PRI) {
        _queue_arr[__pri_heig].Push(msg);

    } else if (msg->_head._flag & HPF_HIGHEST_PRI) {
        _queue_arr[__pri_heighest].Push(msg);

    }
    _size++;
    _notify.notify_all();
}

NetMsg* CPriorityQueue::Pop() {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _notify.wait(_mutex, [this]() {return this->_size.load() != 0; });
    }

    _size--;
    NetMsg* msg;
    if (_pri_heighest_count > 0) {
        if (_queue_arr[__pri_heighest].Pop(msg)) {
            _pri_heighest_count--;
            return msg;
        }
    }
    if (_pri_heig_count > 0) {
        if (_queue_arr[__pri_heig].Pop(msg)) {
            _pri_heig_count--;
            _pri_heighest_count = __pri_surplus;
            return msg;
        }
    }
    if (_pri_normal_count > 0) {
        if (_queue_arr[__pri_normal].Pop(msg)) {
            _pri_normal_count--;
            _pri_heig_count = __pri_surplus;
            _pri_heighest_count = __pri_surplus;
            return msg;
        }
    }

    _pri_normal_count = __pri_surplus;
    _pri_heig_count = __pri_surplus;
    _pri_heighest_count = __pri_surplus;

    if (_queue_arr[__pri_low].Pop(msg)) {
        return msg;
    }

    base::LOG_ERROR("can't get message from _queue_arr. shouldn't be here.");
    return nullptr;
}

uint64_t CPriorityQueue::Size() {
    return _size.load();
}

void CPriorityQueue::Clear() {
    for (size_t i = 0; i < __pri_queue_size; i++) {
        _queue_arr[i].Clear();
    }
}