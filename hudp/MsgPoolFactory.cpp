#include "IMsg.h"
#include "Timer.h"
#include "MsgImpl.h"
#include "MsgPoolFactory.h"
using namespace hudp;

#include <iostream>

// if bigger than it, will reduce half of queue size
static const uint16_t __reduce_limit_size = 10;

static void SharedPtrDeleteer(CMsg* msg) {
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
    return std::shared_ptr<CMsg>(CreateMsg(), SharedPtrDeleteer);
}

CMsg* CMsgPoolFactory::CreateMsg() {
    CMsg* net_msg;
    if (!_free_net_msg_queue.Pop(net_msg)) {
        net_msg = new CMsgImpl();
    }

    static int a = 0;
    a++;
    std::cout << "create msg count : " << a << std::endl;
    if (net_msg) {
        return net_msg;
    }
    return nullptr;
}

void CMsgPoolFactory::DeleteMsg(CMsg* msg) {
    msg->Clear();
    _free_net_msg_queue.Push(msg);
    static int a = 0;
    a++;
    std::cout << "delete msg count : " << a << std::endl;
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