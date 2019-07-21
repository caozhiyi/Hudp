#include "SerializesFilter.h"
#include "Log.h"
#include "BitStreamPool.h"
#include "NetMsgPool.h"
#include "Serializes.h"
#include "CommonType.h"
#include "HudpImpl.h"
#include "FunctionNetMsg.h"

using namespace hudp;

bool CSerializesFilter::OnSend(NetMsg* msg) {
    if (msg->_bit_stream) {
        if (msg->_change) {
            msg->_bit_stream->Clear();
            CBitStreamWriter* temp_bit_stream = static_cast<CBitStreamWriter*>(msg->_bit_stream);
            if (CSerializes::Serializes(*msg, *temp_bit_stream)) {
                msg->_bit_stream = temp_bit_stream;
                CHudpImpl::Instance().SendMsgToNet(msg);
            
            } else {
                base::LOG_ERROR("serializes msg to stream failed. id : %d, handle : %s", msg->_head._id, msg->_ip_port.c_str());
                return false;
            }
        }
    }
    CBitStreamWriter* temp_bit_stream = static_cast<CBitStreamWriter*>(CBitStreamPool::Instance().GetBitStream());

    if (CSerializes::Serializes(*msg, *temp_bit_stream)) {
        msg->_bit_stream = temp_bit_stream;
        CHudpImpl::Instance().SendMsgToNet(msg);

    } else {
        base::LOG_ERROR("serializes msg to stream failed. id : %d, handle : %s", msg->_head._id, msg->_ip_port.c_str());
        return false;
    }
   
    return true;
}

bool CSerializesFilter::OnRecv(NetMsg* msg) {
    CBitStreamReader* temp_bit_stream = static_cast<CBitStreamReader*>(msg->_bit_stream);
   
    if (CSerializes::Deseriali(*temp_bit_stream, *msg)) {
        static_cast<CReceiverNetMsg*>(msg)->NextPhase();

    } else {
        base::LOG_ERROR("deserialize stream to msg failed. id : %d, handle : %s", msg->_head._id, msg->_ip_port.c_str());
        return false;
    }
    return true;
}