#include "Log.h"
#include "IMsg.h"
#include "HudpImpl.h"
#include "CommonFlag.h"
#include "HudpConfig.h"
#include "PriorityQueue.h"

using namespace hudp;

CPriorityQueueImpl::CPriorityQueueImpl() : _pri_normal_count(__pri_surplus),
                                           _pri_high_count(__pri_surplus),
                                           _pri_highest_count(__pri_surplus) {

}

CPriorityQueueImpl::~CPriorityQueueImpl() {
    Clear();
}

void CPriorityQueueImpl::PushBack(std::shared_ptr<CMsg> msg) {
    uint16_t flag = msg->GetFlag();
    if (flag & HPF_LOW_PRI) {
        _queue_arr[__pri_low].push(msg);

    } else if (flag & HPF_NROMAL_PRI) {
        _queue_arr[__pri_normal].push(msg);

    } else if (flag & HPF_HIGH_PRI) {
        _queue_arr[__pri_high].push(msg);

    } else if (flag & HPF_HIGHEST_PRI) {
        _queue_arr[__pri_highest].push(msg);
    }
}

std::shared_ptr<CMsg> CPriorityQueueImpl::Pop() {
    std::shared_ptr<CMsg> msg;
    if (_pri_highest_count > 0) {
        if (!_queue_arr[__pri_highest].empty()) {
            msg = _queue_arr[__pri_highest].front();
            _queue_arr[__pri_highest].pop();
            _pri_highest_count--;
            return msg;
        }
    }
    if (_pri_high_count > 0) {
        if (!_queue_arr[__pri_high].empty()) {
            msg = _queue_arr[__pri_high].front();
            _queue_arr[__pri_high].pop();
            _pri_highest_count = __pri_surplus;
            _pri_high_count--;
            return msg;
        }
    }
    if (_pri_normal_count > 0) {
        if (!_queue_arr[__pri_normal].empty()) {
            msg = _queue_arr[__pri_normal].front();
            _queue_arr[__pri_normal].pop();
            _pri_high_count = __pri_surplus;
            _pri_highest_count = __pri_surplus;
            _pri_normal_count--;
            return msg;
        }
    }

    _pri_normal_count = __pri_surplus;
    _pri_high_count = __pri_surplus;
    _pri_highest_count = __pri_surplus;

    if (!_queue_arr[__pri_low].empty()) {
        msg = _queue_arr[__pri_low].front();
        _queue_arr[__pri_low].pop();
        return msg;

    // if don't have pri low
    } else {
        if (_pri_highest_count > 0) {
            if (!_queue_arr[__pri_highest].empty()) {
                msg = _queue_arr[__pri_highest].front();
                _queue_arr[__pri_highest].pop();
                _pri_highest_count--;
                return msg;
            }
        }
        if (_pri_high_count > 0) {
            if (!_queue_arr[__pri_high].empty()) {
                msg = _queue_arr[__pri_high].front();
                _queue_arr[__pri_high].pop();
                _pri_highest_count = __pri_surplus;
                _pri_high_count--;
                return msg;
            }
        }
        if (_pri_normal_count > 0) {
            if (!_queue_arr[__pri_normal].empty()) {
                msg = _queue_arr[__pri_normal].front();
                _queue_arr[__pri_normal].pop();
                _pri_high_count = __pri_surplus;
                _pri_highest_count = __pri_surplus;
                _pri_normal_count--;
                return msg;
            }
        }
    }

    return nullptr;
}

uint64_t CPriorityQueueImpl::Size() {
    uint64_t ret = 0;
    for (auto i = 0; i < __pri_queue_size; i++) {
        ret += _queue_arr[i].size();
    }
    return ret;
}

void CPriorityQueueImpl::Clear() {
    for (size_t i = 0; i < __pri_queue_size; i++) {
        std::shared_ptr<CMsg> msg;
        while (!_queue_arr[i].empty()) {
            // return to msg pool
            msg = _queue_arr[i].front();
            _queue_arr[i].pop();
            msg.reset();
        }
    }
}
