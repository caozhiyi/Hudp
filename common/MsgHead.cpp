#include "MsgHead.h"
#include "Log.h"
#include "CommonFlag.h"

using namespace hudp;

Head::Head() : _flag(0),
               _id(0),
               _body_len(0),
               _ack_reliable_len(0),
               _ack_reliable_orderly_len(0) {

}

Head::~Head() {

}

void Head::Clear() {
    _flag = 0;
    _id = 0;
    _body_len = 0;
    _ack_reliable_len = 0;
    _ack_reliable_orderly_len = 0;
    _ack_reliable_vec.clear();
    _ack_reliable_orderly_vec.clear();
}

void Head::ClearAck() {
    _ack_reliable_len = 0;
    _ack_reliable_orderly_len = 0;
    _ack_reliable_vec.clear();
    _ack_reliable_orderly_vec.clear();
    _flag &= ~HPF_WITH_RELIABLE_ACK;
    _flag &= ~HPF_WITH_RELIABLE_ORDERLY_ACK;
    _flag &= ~HPF_RELIABLE_ACK_RANGE;
    _flag &= ~HPF_RELIABLE_ORDERLY_ACK_RANGE;
}

void Head::SetBodyLength(uint16_t length) {
    _body_len = length;
    _flag |= HPF_WITH_BODY;
}

uint16_t Head::GetBodyLength() {
    return _body_len;
}

void Head::SetId(uint16_t id) {
    _id = id;
    _flag |= HPF_WITH_ID;
}

uint16_t Head::GetId() {
    return _id;
}

bool Head::AddReliableOrderlyAck(const std::vector<uint16_t>& ack_vec, bool continue_range) {
    if (_ack_reliable_orderly_vec.size() > 0) {
        base::LOG_ERROR("AddReliableOrderlyAck only can called once");
        return false;
    }
    if (ack_vec.empty()) {
        base::LOG_ERROR("ack vector is empty.");
        return false;
    }

    _ack_reliable_orderly_len = (uint16_t)ack_vec.size();

    if (continue_range) {
        _ack_reliable_orderly_vec.push_back(ack_vec[0]);
        _flag |= HPF_RELIABLE_ORDERLY_ACK_RANGE;
    
    } else {
        for (auto ack : ack_vec) {
            _ack_reliable_orderly_vec.push_back(ack);
        }
    }

    _flag |= HPF_WITH_RELIABLE_ORDERLY_ACK;
    return true;
}

bool Head::GetReliableOrderlyAck(std::vector<uint16_t>& ack_vec) {
    if (_ack_reliable_orderly_vec.empty()) {
        return false;
    }

    if (_flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {
        uint16_t base_ack = _ack_reliable_orderly_vec[0];
        for (uint16_t index = 0; index < _ack_reliable_orderly_len; index++) {
            ack_vec.push_back(base_ack + index);
        }

    } else {
        ack_vec = std::move(_ack_reliable_orderly_vec);
    }
    return true;
}

bool Head::AddReliableAck(const std::vector<uint16_t>& ack_vec, bool continue_range) {
    if (_ack_reliable_vec.size() > 0) {
        base::LOG_ERROR("AddReliableAck only can called once");
        return false;
    }
    if (ack_vec.empty()) {
        base::LOG_ERROR("ack vector is empty.");
        return false;
    }

    _ack_reliable_len = (uint16_t)ack_vec.size();

    if (continue_range) {
        _ack_reliable_vec.push_back(ack_vec[0]);
        _flag |= HPF_RELIABLE_ACK_RANGE;

    } else {
        for (auto ack : ack_vec) {
            _ack_reliable_vec.push_back(ack);
        }
    }

    _flag |= HPF_WITH_RELIABLE_ACK;
    return true;
}

bool Head::GetReliableAck(std::vector<uint16_t>& ack_vec) {
    if (_ack_reliable_vec.empty()) {
        return false;
    }

    if (_flag & HPF_RELIABLE_ACK_RANGE) {
        uint16_t base_ack = _ack_reliable_vec[0];
        for (uint16_t index = 0; index < _ack_reliable_orderly_len; index++) {
            ack_vec.push_back(base_ack + index);
        }

    } else {
        ack_vec = std::move(_ack_reliable_vec);
    }
    return true;
}