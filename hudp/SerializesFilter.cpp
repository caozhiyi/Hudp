#include "SerializesFilter.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "Serializes.h"
#include "CommonType.h"

using namespace hudp;

bool CSerializesFilter::OnSend(NetMsg* msg) {
    CBitStreamWriter* temp_bit_stream = static_cast<CBitStreamWriter*>(CBitStreamPool::Instance().GetBitStream());

    CHECK_RET(CSerializes::Serializes(*msg, *temp_bit_stream));
    
    msg->_bit_stream = temp_bit_stream;

    return true;
}

bool CSerializesFilter::OnRecv(NetMsg* msg) {
    CBitStreamReader* temp_bit_stream = static_cast<CBitStreamReader*>(msg->_bit_stream);
    CHECK_RET(CSerializes::Deseriali(*temp_bit_stream, *msg));

    return true;
}