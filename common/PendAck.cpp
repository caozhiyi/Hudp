#include "PendAck.h"

using namespace hudp;

CPendAck::CPendAck() : _ack_len(0), _ack_start(-1), _expect_start_id(0) {

}

CPendAck::~CPendAck() {

}

void CPendAck::AddAck(uint16_t ack_id) {
    std::unique_lock<std::mutex> lock(_mutex);

    // the first ack
    if(_ack_set.size() == 0) {
        _ack_start = ack_id;
        _ack_len = 1;
        _expect_start_id = _ack_start + _ack_len;

    // is consecutive ack id
    } else if (ack_id == _expect_start_id) {
        _ack_start = ack_id;
        CheckConsecutive();

    }
    // isn't consecutive ack id, only insert to set.

    _ack_set.insert(ack_id);
}

bool CPendAck::GetAllAck(std::vector<uint16_t>& ack_vec) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_ack_set.empty()) {
        return false;
    }
    
    for (auto iter = _ack_set.begin(); iter != _ack_set.end(); ++iter) {
        ack_vec.push_back(*iter);
    }

    _ack_set.clear();
    _ack_start = -1;
    _ack_len = 0;
    return true;
}

bool CPendAck::GetConsecutiveAck(uint16_t& start, uint16_t& len) {
    std::unique_lock<std::mutex> lock(_mutex);

    if (_ack_start == -1) {
        return false;
    }

    start = _ack_start;
    len = _ack_len;
    for (uint16_t i = _ack_start; i < _ack_len; i++) {
        _ack_set.erase(i);
    }
    _ack_start = -1;
    _ack_len = 0;

    return true;
}

void CPendAck::CheckConsecutive() {
    for (auto iter = _ack_set.begin(); iter != _ack_set.end(); ++iter) {
        if (*iter == _ack_start + _ack_len) {
            _ack_len += 1;
        
        } else {
            break;
        }
    }
}