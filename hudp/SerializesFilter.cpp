#include "SerializesFilter.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "Serializes.h"
#include "CommonType.h"

using namespace hudp;

bool CSerializesFilter::OnSend(NetMsg* msg, CHudpBitStream* bit_stream, const std::string& _ip_port) {
    CHudpBitStream* temp_bit_stream = CBitStreamPool::Instance().GetBitStream();

    CHECK_RET(CSerializes::Serializes(*msg, *temp_bit_stream));
    CNetMsgPool::Instance().FreeMsg(msg);

    msg = nullptr;
    bit_stream = temp_bit_stream;

    return true;
}

bool CSerializesFilter::OnRecv(CHudpBitStream* bit_stream, NetMsg* msg, const std::string& _ip_port) {
    NetMsg* temp_msg = CNetMsgPool::Instance().GetMsg();

    CHECK_RET(CSerializes::Deseriali(*bit_stream, *temp_msg));
    CBitStreamPool::Instance().FreeBitStream(bit_stream);

    bit_stream = nullptr;
    msg = temp_msg;

    return true;
}