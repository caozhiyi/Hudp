#include "MsgImpl.h"
#include "CommonFlag.h"

using namespace hudp;
CMsgImpl::CMsgImpl() : _push_send_time(0),
                       _backoff_factor(1),
                       _flag(msg_with_out_id) {
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

void CMsgImpl::SetFlag(uint16_t flag) {
    _head._flag = flag;
}

uint16_t CMsgImpl::GetFlag() {
    return _head._flag;
}

void CMsgImpl::SetHandle(const Handle& handle) {
    _ip_port = handle;
}

const Handle& CMsgImpl::GetHandle() {
    return _ip_port;
}

void CMsgImpl::SetBody(const std::string& body) {
    _body = body;
    _head.SetBodyLength(body.length());
}

std::string& CMsgImpl::GetBody() {
    return _body;
}

bool CMsgImpl::GetSerializeBuffer(char* buf, uint16_t& len) {
    return true;
}