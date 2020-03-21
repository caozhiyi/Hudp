#include "Log.h"
#include "MsgHead.h"
#include "CommonFlag.h"

using namespace hudp;

Head::Head() : _flag(0),
               _id(0),
               _body_len(0),
               _send_time(0) {

}

Head::~Head() {

}

void Head::Clear() {
    _flag = 0;
    _id = 0;
    _body_len = 0;
    _ack_reliable_vec.clear();
    _ack_reliable_orderly_vec.clear();
    _reliable_msg_send_time_vec.clear();
    _reliable_orderly_msg_send_time_vec.clear();
}

void Head::ClearAck() {
    _ack_reliable_vec.clear();
    _ack_reliable_orderly_vec.clear();
    _reliable_msg_send_time_vec.clear();
    _reliable_orderly_msg_send_time_vec.clear();
    _flag &= ~HPF_WITH_RELIABLE_ACK;
    _flag &= ~HPF_WITH_RELIABLE_ORDERLY_ACK;
    _flag &= ~HPF_RELIABLE_ACK_RANGE;
    _flag &= ~HPF_RELIABLE_ORDERLY_ACK_RANGE;
}

void Head::SetFlag(uint32_t flag) {
    _flag = flag;
}

uint32_t Head::GetFlag() {
    return _flag;
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

void Head::SetSendTime(uint64_t time) {
    _send_time = time;
}

uint64_t Head::GetSendTime() {
    return _send_time;
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

    _ack_reliable_orderly_vec = ack_vec;

    if (continue_range) {
        _flag |= HPF_RELIABLE_ORDERLY_ACK_RANGE;
    }

    _flag |= HPF_WITH_RELIABLE_ORDERLY_ACK;
    return true;
}

bool Head::GetReliableOrderlyAck(std::vector<uint16_t>& ack_vec) {
    if (_ack_reliable_orderly_vec.empty()) {
        return false;
    }

    ack_vec = _ack_reliable_orderly_vec;
    return true;
}

bool Head::AddReliableOrderlyAckTime(const std::vector<uint64_t>& time_vec) {
    if (_reliable_orderly_msg_send_time_vec.size() > 0) {
        base::LOG_ERROR("AddReliableOrderlyAckTime only can called once");
        return false;
    }
    if (time_vec.empty()) {
        base::LOG_ERROR("time vector is empty.");
        return false;
    }

    _reliable_orderly_msg_send_time_vec = time_vec;
    _flag |= HPF_MSG_WITH_TIME_STAMP;
    return true;
}

bool Head::GetReliableOrderlyAckTime(std::vector<uint64_t>& time_vec) {
    if (_reliable_orderly_msg_send_time_vec.empty()) {
        return false;
    }

    time_vec = _reliable_orderly_msg_send_time_vec;
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

    _ack_reliable_vec = ack_vec;

    if (continue_range) {
        _flag |= HPF_RELIABLE_ACK_RANGE;
    }

    _flag |= HPF_WITH_RELIABLE_ACK;
    return true;
}

bool Head::GetReliableAck(std::vector<uint16_t>& ack_vec) {
    if (_ack_reliable_vec.empty()) {
        return false;
    }

    ack_vec = _ack_reliable_vec;
    return true;
}

bool Head::AddReliableAckTime(const std::vector<uint64_t>& time_vec) {
    if (_reliable_msg_send_time_vec.size() > 0) {
        base::LOG_ERROR("AddReliableAckTime only can called once");
        return false;
    }
    if (time_vec.empty()) {
        base::LOG_ERROR("time vector is empty.");
        return false;
    }

    _reliable_msg_send_time_vec = time_vec;
    _flag |= HPF_MSG_WITH_TIME_STAMP;
    return true;
}

bool Head::GetReliableAckTime(std::vector<uint64_t>& time_vec) {
    if (_reliable_msg_send_time_vec.empty()) {
        return false;
    }

    time_vec = _reliable_msg_send_time_vec;
    return true;
}