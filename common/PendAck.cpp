#include "Log.h"
#include "PendAck.h"
#include "HudpConfig.h"

using namespace hudp;

static uint16_t __id_length   = sizeof(uint16_t);
static uint16_t __time_length = sizeof(uint64_t);
static uint16_t __ack_length_limit = __mtu - __time_length - __id_length; // reserved buffer

CPendAck::CPendAck() : _cur_size(0) {

}

CPendAck::~CPendAck() {

}

void CPendAck::AddAck(uint16_t ack_id) {
    base::LOG_DEBUG("[ACK] : pend ack add a msg. id : %d", ack_id);
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _ack_map[ack_id] = 0;
        _cur_size += __id_length;
    }
    if (_cur_size >= __ack_length_limit) {
        _ack_now_call_back();
    }
}

void CPendAck::AddAck(uint16_t ack_id, uint64_t send_time) {
    base::LOG_DEBUG("[ACK] : pend ack add a msg. id : %d", ack_id);
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _ack_map[ack_id] = send_time;
        _cur_size += __id_length + __time_length;
    }
    if (_cur_size >= __ack_length_limit) {
        _ack_now_call_back();
    }
}

void CPendAck::Clear() {
    std::unique_lock<std::mutex> lock(_mutex);
    _ack_map.clear();
}

bool CPendAck::GetAck(std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool& continuity, uint16_t size_limit) {
    uint16_t ack_count = 0;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_ack_map.empty()) {
            return false;
        }
        ack_count = (uint16_t)_ack_map.size();
    }
    
    std::unique_lock<std::mutex> lock(_mutex);
    uint16_t prev_tmp = 0;
    uint16_t cur_szie = 0;
    while (1) {
        auto iter = _ack_map.begin();
        if (iter == _ack_map.end()) {
            break;
        }
        
        if (iter->second != 0) {
            cur_szie += __time_length;
            if (size_limit <= cur_szie) {
                break;
            }
            cur_szie += __id_length;
            if (size_limit <= cur_szie) {
                break;
            }
            ack_vec.push_back(iter->first);
            time_vec.push_back(iter->second);

        } else {
            cur_szie += __id_length;
            if (size_limit <= cur_szie) {
                break;
            }
            ack_vec.push_back(iter->first);
        }

        if (prev_tmp != 0 && prev_tmp != (uint16_t)(iter->first - 1)) {
            continuity = false;
        }
        prev_tmp = iter->first;
        _ack_map.erase(iter);
    }

    // only ack don't need continuity
    if (continuity && ack_vec.size() == 1) {
        continuity = false;
    }
    base::LOG_DEBUG("[ACK] : attach ack msg. id : %d, size : %d", ack_vec[0], ack_vec.size());
    _cur_size -= cur_szie;
    return true;
}

bool CPendAck::GetAllAck(std::vector<uint16_t>& ack_vec, bool& continuity) {
    std::vector<uint64_t> time_vec;
    return GetAllAck(ack_vec, time_vec, continuity);
}

bool CPendAck::GetAllAck(std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool& continuity) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_ack_map.empty()) {
        return false;
    }

    continuity = true;

    uint16_t len = (uint16_t)_ack_map.size();
    uint16_t prev_tmp = 0;
    while (1) {
        auto iter = _ack_map.begin();
        if (iter == _ack_map.end()) {
            break;
        }

        ack_vec.push_back(iter->first);

        if (iter->second != 0) {
            time_vec.push_back(iter->second);
        }

        if (prev_tmp != 0 && prev_tmp != (uint16_t)(iter->first - 1)) {
            continuity = false;
        }
        prev_tmp = iter->first;
        _ack_map.erase(iter);
    }

    // only ack don't need continuity
    if (continuity && ack_vec.size() == 1) {
        continuity = false;
    }
    base::LOG_DEBUG("[ACK] : attach ack msg. id : %d, size : %d", ack_vec[0], ack_vec.size());
    _ack_map.clear();
    _cur_size = 0;
    return true;
}

bool CPendAck::HasAck() {
    return !_ack_map.empty();
}

void CPendAck::SetSendAckNowCallBack(std::function<void()> cb) {
    _ack_now_call_back = cb;
}