#include "SerializesFilter.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "Serializes.h"
#include "CommonType.h"

using namespace hudp;

bool CSerializesFilter::OnSend(NetMsg* msg, CBitStream* bit_stream, const std::string& _ip_port) {
    CBitStreamWriter* temp_bit_stream = static_cast<CBitStreamWriter*>(CBitStreamPool::Instance().GetBitStream());

    CHECK_RET(CSerializes::Serializes(*msg, *temp_bit_stream));
    CNetMsgPool::Instance().FreeMsg(msg);

    msg = nullptr;
    bit_stream = temp_bit_stream;

    return true;
}

bool CSerializesFilter::OnRecv(CBitStream* bit_stream, NetMsg* msg, const std::string& _ip_port) {
    NetMsg* temp_msg = CNetMsgPool::Instance().GetMsg();
    CBitStreamReader* temp_bit_stream = static_cast<CBitStreamReader*>(bit_stream);
    CHECK_RET(CSerializes::Deseriali(*temp_bit_stream, *temp_msg));
    CBitStreamPool::Instance().FreeBitStream(bit_stream);

    bit_stream = nullptr;
    msg = temp_msg;

    return true;
}