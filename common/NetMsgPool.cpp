#include "NetMsgPool.h"
using namespace hudp;

CNetMsgPool::CNetMsgPool() {
    ExpendFree();
}

CNetMsgPool::~CNetMsgPool() {
    size_t size = _free_queue.Size();
    NetMsg* msg;
    for (int i = 0; i < __init_pool_size; i++) {
        _free_queue.Pop(msg);
        delete msg;
    }
}

void CNetMsgPool::ExpendFree() {
    NetMsg* msg;
    for (int i = 0; i < __init_pool_size; i++) {
        msg = new NetMsg();
        _free_queue.Push(msg);
    }
}

void CNetMsgPool::ReduceFree() {
    size_t size = _free_queue.Size() / 2;
    NetMsg* msg;
    for (int i = 0; i < __init_pool_size; i++) {
        _free_queue.Pop(msg);
        delete msg;
    }
}

NetMsg* CNetMsgPool::GetMsg() {
    NetMsg* msg = nullptr;
    _free_queue.Pop(msg);
    return msg;
}

void CNetMsgPool::FreeMsg(NetMsg* msg) {
    msg->Clear();
    _free_queue.Push(msg);
}