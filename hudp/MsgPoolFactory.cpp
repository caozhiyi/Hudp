#include "IMsg.h"
#include "Timer.h"
#include "MsgImpl.h"
#include "HudpConfig.h"
#include "MsgPoolFactory.h"
using namespace hudp;

static void SharedPtrDeleter(CMsg* msg) {
    // delete from timer.
    if (msg->GetTimerId() > 0) {
        CTimer::Instance().RemoveTimer(msg);
    }
    CMsgPoolFactory::Instance().DeleteMsg(msg);
}

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

std::shared_ptr<CMsg> CMsgPoolFactory::CreateSharedMsg() {
    return std::shared_ptr<CMsg>(CreateMsg(), SharedPtrDeleter);
}

CMsg* CMsgPoolFactory::CreateMsg() {
    CMsg* net_msg;
    if (!_free_net_msg_queue.Pop(net_msg)) {
        net_msg = new CMsgImpl();
    }

    return net_msg;
}

void CMsgPoolFactory::DeleteMsg(CMsg* msg) {
    msg->Clear();
    _free_net_msg_queue.Push(msg);
    ReduceFree();
}

void CMsgPoolFactory::ReduceFree() {
    size_t size = _free_net_msg_queue.Size();
    if (size > __msg_pool_reduce_limit_size) {
        size = size / 2;
        CMsg* net_msg;
        for (size_t i = 0; i < size; i++) {
            if (_free_net_msg_queue.Pop(net_msg)) {
                delete net_msg;
            }
        }
    }
}
