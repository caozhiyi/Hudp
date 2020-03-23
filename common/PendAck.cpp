#include "Log.h"
#include "PendAck.h"

using namespace hudp;

CPendAck::CPendAck() {

}

CPendAck::~CPendAck() {

}

void CPendAck::AddAck(uint16_t ack_id) {
    base::LOG_DEBUG("[ACK] : pend ack add a msg. id : %d", ack_id);
    std::unique_lock<std::mutex> lock(_mutex);
    if (_ack_set.empty()) {
        _start = ack_id;
    }
    _ack_set[ack_id] = 0;
}

void CPendAck::AddAck(uint16_t ack_id, uint64_t send_time) {
    base::LOG_DEBUG("[ACK] : pend ack add a msg. id : %d", ack_id);
    std::unique_lock<std::mutex> lock(_mutex);
    if (_ack_set.empty()) {
        _start = ack_id;
    }
    _ack_set[ack_id] = send_time;
}

bool CPendAck::GetAllAck(std::vector<uint16_t>& ack_vec, bool& continuity) {
    std::unique_lock<std::mutex> lock(_mutex);
    std::vector<uint64_t> time_vec;
    return GetAllAck(ack_vec, time_vec, continuity);
}

bool CPendAck::GetAllAck(std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool& continuity) {
        std::unique_lock<std::mutex> lock(_mutex);
    if (_ack_set.empty()) {
        return false;
    }

    continuity = true;

    uint16_t len = (uint16_t)_ack_set.size();
    auto iter = _ack_set.find(_start);
    ack_vec.push_back(iter->first);
    time_vec.push_back(iter->second);
    uint16_t prev_tmp = iter->first;
    while (1) {
        iter++;
        len--;
        if (len == 0) {
            break;
        }

        if (iter == _ack_set.end()) {
            iter = _ack_set.begin();
        }

        if (prev_tmp != (uint16_t)(iter->first - 1)) {
            continuity = false;
        }
        prev_tmp = iter->first;
        
        ack_vec.push_back(iter->first);
        time_vec.push_back(iter->second);
    }

    // only ack don't need continuity
    if (continuity && ack_vec.size() == 1) {
        continuity = false;
    }
    base::LOG_DEBUG("[ACK] : attach ack msg. id : %d, size : %d", ack_vec[0], ack_vec.size());
    _ack_set.clear();
    return true;
}

bool CPendAck::HasAck() {
    return !_ack_set.empty();
}
