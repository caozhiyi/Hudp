#include "Serializes.h"
#include "Log.h"
#include "CommonFlag.h"
#include "NetMsg.h"
#include "BitStream.h"

using namespace hudp;

bool CSerializes::Serializes(NetMsg& msg, CHudpBitStream& bit_stream) {
    return Serializes(msg._head, msg._body, msg._head._body_len, bit_stream);
}

bool CSerializes::Serializes(const Head& head, CHudpBitStream& bit_stream) {
    // must serializes head first
    if (bit_stream.GetCurrentLength() > 0) {
        base::LOG_ERROR("head is not first to be serializes");
        return false;
    }
    
    // fixed sequence by read/write
    bit_stream.Write(head._flag);
    if (head._flag & HPF_WITH_ID) {
        CHECK_RET(bit_stream.Write(head._id));
    }
    if (head._flag & HPF_WITH_ACK_RANGE) {
        CHECK_RET(bit_stream.Write(head._ack_len));
        CHECK_RET(bit_stream.Write(head._ack_start));
    }
    if (head._flag & HPF_WITH_ACK_ARRAY) {
        CHECK_RET(bit_stream.Write(head._ack_len));
        for(auto iter = head._ack_vec.begin(); iter != head._ack_vec.end(); ++iter) {
            CHECK_RET(bit_stream.Write(*iter));
        }
    }
    if (head._flag & HPF_WITH_BODY) {
        CHECK_RET(bit_stream.Write(head._body_len));
    }
    return true;
}

bool CSerializes::Serializes(Head& head, const char* body, uint16_t len, CHudpBitStream& bit_stream) {
    if (len > 0) {
        head._body_len = len;
        head._flag |= HPF_WITH_BODY;
    }
    if (!Serializes(head, bit_stream)) {
        return false;
    }
    
    return bit_stream.Write(body, len);
}

bool CSerializes::Deseriali(CHudpBitStream& bit_stream, NetMsg& msg) {
    return Deseriali(bit_stream, msg._head, msg._body, msg._head._body_len);
}

bool CSerializes::Deseriali(CHudpBitStream& bit_stream, Head& head) {
    
    // fixed sequence by read/write
    bit_stream.Read(head._flag);
    if (head._flag & HPF_WITH_ID) {
        CHECK_RET(bit_stream.Read(head._id));
    }
    if (head._flag & HPF_WITH_ACK_RANGE) {
        CHECK_RET(bit_stream.Read(head._ack_len));
        CHECK_RET(bit_stream.Read(head._ack_start));
    }
    if (head._flag & HPF_WITH_ACK_ARRAY) {
        CHECK_RET(bit_stream.Read(head._ack_len));
        for (auto iter = head._ack_vec.begin(); iter != head._ack_vec.end(); ++iter) {
            CHECK_RET(bit_stream.Read(*iter));
        }
    }
    if (head._flag & HPF_WITH_BODY) {
        CHECK_RET(bit_stream.Read(head._body_len));
    }
}
  
bool CSerializes::Deseriali(CHudpBitStream& bit_stream, Head& head, char* body, uint16_t& len) {
    if (!Deseriali(bit_stream, head)) {
        return false;
    }
    CHECK_RET(bit_stream.Read(body, head._body_len));
    len = head._body_len;
    return true;
}