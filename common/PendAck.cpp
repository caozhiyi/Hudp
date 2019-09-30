#include "PendAck.h"
#include "Log.h"
using namespace hudp;

CPendAck::CPendAck() {

}

CPendAck::~CPendAck() {

}

void CPendAck::AddAck(uint16_t ack_id) {
    base::LOG_DEBUG("[ACK] : pend ack add a msg. id : %d", ack_id);
    std::unique_lock<std::mutex> lock(_mutex);
    _ack_set.insert(ack_id);
}

bool CPendAck::GetAllAck(std::vector<uint16_t>& ack_vec, bool& continuity) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_ack_set.empty()) {
        return false;
    }

    continuity = true;

    auto iter = _ack_set.begin();
    uint16_t tmp = *iter;

    ack_vec.push_back(*iter);
    ++iter;
    for (; iter != _ack_set.end(); ++iter) {
        if (tmp != *iter - 1) {
            continuity = false;
        }
        tmp = *iter;
        
        ack_vec.push_back(*iter);
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
