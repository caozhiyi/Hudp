#include "MsgImpl.h"
#include "CommonFlag.h"
#include "BitStream.h"
#include "Serializes.h"

using namespace hudp;
CMsgImpl::CMsgImpl() : _push_send_time(0),
                       _backoff_factor(1),
                       _flag(msg_with_out_id),
                       _next(nullptr),
                       _prev(nullptr) {

}

CMsgImpl::~CMsgImpl() {

}

void CMsgImpl::Clear() {
    _flag = msg_with_out_id;
    _ip_port.clear();
    _head.Clear();
    _socket.reset();
    _backoff_factor = 1;
    _push_send_time = 0;
}

void CMsgImpl::ClearAck() {
    _head.ClearAck();
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

void CMsgImpl::TranslateFlag() {
    if (_head._flag & HTF_ORDERLY) {
        _head._flag &= ~HTF_ORDERLY;
        _head._flag |= HPF_IS_ORDERLY;

    } else if (_head._flag & HTF_RELIABLE) {
        _head._flag &= ~HTF_RELIABLE;
        _head._flag |= HPF_NEED_ACK;

    } else if (_head._flag & HTF_RELIABLE_ORDERLY) {
        _head._flag &= ~HTF_RELIABLE_ORDERLY;
        _head._flag |= HPF_NEED_ACK;
        _head._flag |= HPF_IS_ORDERLY;

    } else if (_head._flag & HTF_NORMAL) {
        _head._flag &= ~HTF_NORMAL;
    }
}

void CMsgImpl::SetHeaderFlag(uint16_t flag) {
    _head._flag = flag;
}

uint16_t CMsgImpl::GetHeaderFlag() {
    return _head._flag;
}

void CMsgImpl::SetFlag(uint16_t flag) {
    _head._flag = flag;
}

uint16_t CMsgImpl::GetFlag() {
    return _head._flag;
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

void CMsgImpl::SetAck(int16_t flag, std::vector<uint16_t>& ack_vec, bool continuity) {
    if (flag & HTF_RELIABLE) {
        _head.AddReliableAck(ack_vec, continuity);
    }
    if (flag & HTF_RELIABLE_ORDERLY) {
        _head.AddReliableOrderlyAck(ack_vec, continuity);
    }
}

void CMsgImpl::GetAck(int16_t flag, std::vector<uint16_t>& ack_vec) {
    if (flag & HTF_RELIABLE) {
        _head.GetReliableAck(ack_vec);
    }
    if (flag & HTF_RELIABLE_ORDERLY) {
        _head.GetReliableOrderlyAck(ack_vec);
    }
}

std::string CMsgImpl::GetSerializeBuffer() {
    CBitStreamWriter bit_stream;
    if (!CSerializes::Serializes(*this, bit_stream)) {
        return "";
    }
    return std::string(bit_stream.GetDataPoint(), bit_stream.GetCurrentLength());
}

bool CMsgImpl::InitWithBuffer(std::string& msg) {
    CBitStreamReader bit_stream;
    bit_stream.Init(msg.c_str(), msg.length());
    if (!CSerializes::Deseriali(bit_stream, *this)) {
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

std::shared_ptr<CSocket> CMsgImpl::GetSocket() {
    return _socket.lock();
}