#include "BitStreamPool.h"
using namespace hudp;

CBitStreamPool::CBitStreamPool() {
    ExpendFree();
}

CBitStreamPool::~CBitStreamPool() {
    size_t size = _free_queue.Size();
    CHudpBitStream* bit_stream;
    for (int i = 0; i < size; i++) {
        _free_queue.Pop(bit_stream);
        delete bit_stream;
    }
}

void CBitStreamPool::ExpendFree() {
    CHudpBitStream* bit_stream;
    for (int i = 0; i < __init_pool_size; i++) {
        bit_stream = new CHudpBitStream();
        _free_queue.Push(bit_stream);
    }
}

void CBitStreamPool::ReduceFree() {
    size_t size = _free_queue.Size() / 2;
    CHudpBitStream* bit_stream;
    for (int i = 0; i < size; i++) {
        _free_queue.Pop(bit_stream);
        delete bit_stream;
    }
}

CHudpBitStream* CBitStreamPool::GetBitStream() {
    CHudpBitStream* bit_stream = nullptr;
    if (_free_queue.Size() == 0) {
        ExpendFree();
    }
    _free_queue.Pop(bit_stream);
    return bit_stream;
}

void CBitStreamPool::FreeBitStream(CHudpBitStream* bit_stream) {
    bit_stream->Clear();
    _free_queue.Push(bit_stream);
}