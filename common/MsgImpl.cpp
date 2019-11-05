#include "MsgImpl.h"
#include "CommonFlag.h"

using namespace hudp;
CMsgImpl::CMsgImpl() : _push_send_time(0),
                       _backoff_factor(1),
                       _flag(false),
                       _use(true) {
}

CMsgImpl::~CMsgImpl() {

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

void CMsgImpl::SetHandle(const Handle& handle) {
    _ip_port = handle;
}

const Handle& CMsgImpl::GetHandle() {
    return _ip_port;
}

void CMsgImpl::Clear() {
    _flag = false;
    _ip_port.clear();
    _head.Clear();
    _socket.reset();
    _use = false;
    _backoff_factor = 1;
    _push_send_time = 0;
}

void CMsgImpl::ClearAck() {
    _head.ClearAck();
}

void CMsgImpl::SetId(const uint16_t& id) {
    _head.SetId(id);
}
    
