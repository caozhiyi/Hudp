#include "MsgImpl.h"
#include "BitStream.h"
#include "Serializes.h"
#include "HudpConfig.h"
#include "CommonFlag.h"

using namespace hudp;

CSerializes* CMsgImpl::_serializes = new CSerializesNormal();

CMsgImpl::CMsgImpl() : _backoff_factor(1),
                       _flag(msg_with_out_id),
                       _next(nullptr),
                       _prev(nullptr),
                       _time_id(0) {

}

CMsgImpl::~CMsgImpl() {

}

void CMsgImpl::Clear() {
    _flag = msg_with_out_id;
    _ip_port.clear();
    _head.Clear();
    _socket.reset();
    _backoff_factor = 1;
    _time_id = 0;
    _body.clear();
}

void CMsgImpl::ClearAck() {
    _head.ClearAck();
}

Head& CMsgImpl::GetHead() {
    return _head;
}

void CMsgImpl::SetId(const uint16_t& id) {
    _flag |= ~msg_with_out_id;
    _head.SetId(id);
}

uint16_t CMsgImpl::GetId() {
    return _head._id;
}

void CMsgImpl::AddSendDelay() {
    _backoff_factor = _backoff_factor >> 2;
}

uint16_t CMsgImpl::GetReSendTime() {
    return __resend_time * _backoff_factor;
}

void CMsgImpl::SetHeaderFlag(uint16_t flag) {
    _head._flag |= flag;
}

uint16_t CMsgImpl::GetHeaderFlag() {
    return _head._flag;
}

void CMsgImpl::SetFlag(uint16_t flag) {
    _flag |= flag;
}

uint16_t CMsgImpl::GetFlag() {
    return _flag;
}

std::string CMsgImpl::DebugHeaderFlag() {
    std::string ret;
    if (_head._flag & HTF_ORDERLY) {
        ret.append(" HTF_ORDERLY ");
    }
    if (_head._flag & HTF_RELIABLE) {
        ret.append(" HTF_RELIABLE ");
    }
    if (_head._flag & HTF_RELIABLE_ORDERLY) {
        ret.append(" HTF_RELIABLE_ORDERLY ");
    }
    if (_head._flag & HTF_NORMAL) {
        ret.append(" HTF_NORMAL ");
    }
    if (_head._flag & HPF_LOW_PRI) {
        ret.append(" HPF_LOW_PRI ");
    }
    if (_head._flag & HPF_NROMAL_PRI) {
        ret.append(" HPF_NROMAL_PRI ");
    }
    if (_head._flag & HPF_HIGH_PRI) {
        ret.append(" HPF_HIGH_PRI ");
    }
    if (_head._flag & HPF_HIGHEST_PRI) {
        ret.append(" HPF_HIGHEST_PRI ");
    }
    if (_head._flag & HPF_RELIABLE_ACK_RANGE) {
        ret.append(" HPF_RELIABLE_ACK_RANGE ");
    }
    if (_head._flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {
        ret.append(" HPF_RELIABLE_ORDERLY_ACK_RANGE ");
    }
    if (_head._flag & HPF_CLOSE) {
        ret.append(" HPF_CLOSE ");
    }
    if (_head._flag & HPF_CLOSE_ACK) {
        ret.append(" HPF_CLOSE_ACK ");
    }
    if (_head._flag & HPF_WITH_ID) {
        ret.append(" HPF_WITH_ID ");
    }
    if (_head._flag & HPF_WITH_RELIABLE_ACK) {
        ret.append(" HPF_WITH_RELIABLE_ACK ");
    }
    if (_head._flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        ret.append(" HPF_WITH_RELIABLE_ORDERLY_ACK ");
    }
    if (_head._flag & HPF_WITH_BODY) {
        ret.append(" HPF_WITH_BODY ");
    }
    return ret;
}

void CMsgImpl::SetHandle(const HudpHandle& handle) {
    _ip_port = handle;
}

const HudpHandle& CMsgImpl::GetHandle() {
    return _ip_port;
}

void CMsgImpl::SetBody(const std::string& body) {
    _body = body;
    _head.SetBodyLength(body.length());
}

std::string& CMsgImpl::GetBody() {
    return _body;
}

void CMsgImpl::SetAck(int16_t flag, std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool continuity) {
    if (flag & HPF_WITH_RELIABLE_ACK) {
        _head.AddReliableAck(ack_vec, continuity);
        _head.AddReliableAckTime(time_vec);
    }
    if (flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        _head.AddReliableOrderlyAck(ack_vec, continuity);
        _head.AddReliableOrderlyAckTime(time_vec);
    }
}

void CMsgImpl::GetAck(int16_t flag, std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec) {
    if (_head._flag & HPF_WITH_RELIABLE_ACK) {
        _head.GetReliableAck(ack_vec);
        _head.GetReliableAckTime(time_vec);
    }
    if (_head._flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        _head.GetReliableOrderlyAck(ack_vec);
        _head.AddReliableOrderlyAckTime(time_vec);
    }
}

std::string CMsgImpl::GetSerializeBuffer() {
    CBitStreamWriter bit_stream;
    if (!_serializes->Serializes(*this, bit_stream)) {
        return "";
    }
    return std::string(bit_stream.GetDataPoint(), bit_stream.GetCurrentLength());
}

bool CMsgImpl::InitWithBuffer(const std::string& msg) {
    CBitStreamReader bit_stream;
    bit_stream.Init(msg.c_str(), msg.length());
    if (!_serializes->Deseriali(bit_stream, *this)) {
        return false;
    }
    return true;
}

void CMsgImpl::SetNext(CMsg* msg) {
    _next = msg;
}

CMsg* CMsgImpl::GetNext() {
    return _next;
}

void CMsgImpl::SetPrev(CMsg* msg) {
    _prev = msg;
}

CMsg* CMsgImpl::GetPrev() {
    return _prev;
}

void CMsgImpl::SetTimerId(uint64_t id) {
    _time_id = id;
}

uint64_t CMsgImpl::GetTimerId() {
    return _time_id;
}

std::shared_ptr<CSocket> CMsgImpl::GetSocket() {
    return _socket.lock();
}

void CMsgImpl::SetSocket(std::shared_ptr<CSocket>& sock) {
    _socket = sock;
}

void CMsgImpl::SetSendTime(uint64_t time) {
    _head.SetSendTime(time);
    if (__msg_with_time) {
        _head._flag |= HPF_MSG_WITH_TIME_STAMP;
    }
}

uint64_t CMsgImpl::GetSendTime() {
    return _head.GetSendTime();
}