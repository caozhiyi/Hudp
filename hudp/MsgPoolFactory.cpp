#include "MsgPoolFactory.h"
#include "IMsg.h"
#include "MsgImpl.h"
using namespace hudp;

// if bigger than it, will reduce half of queue size
static const uint16_t __reduce_limit_size = 10;

CMsgPoolFactory::CMsgPoolFactory() {

}

CMsgPoolFactory::~CMsgPoolFactory() {
    size_t size = _free_net_msg_queue.Size();
    CMsg* net_msg;
    for (size_t i = 0; i < size; i++) {
        _free_net_msg_queue.Pop(net_msg);
        delete net_msg;
    }
}

CMsg* CMsgPoolFactory::CreateMsg(uint16_t flag) {
    CMsg* net_msg;
    if (!_free_net_msg_queue.Pop(net_msg)) {
        net_msg = new CMsgImpl();
    }

    if (net_msg) {
        net_msg->SetFlag(flag);
        return net_msg;
    }
    return nullptr;
}

void CMsgPoolFactory::DeleteMsg(CMsg* msg) {
    msg->Clear();
    _free_net_msg_queue.Push(msg);
    ReduceFree();
}

void CMsgPoolFactory::ReduceFree() {
    size_t size = _free_net_msg_queue.Size();
    if (size > __reduce_limit_size) {
        size = size / 2;
        CMsg* net_msg;
        for (size_t i = 0; i < size; i++) {
            _free_net_msg_queue.Pop(net_msg);
            delete net_msg;
        }
    }
}