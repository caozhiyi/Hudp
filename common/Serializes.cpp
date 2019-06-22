#include "Serializes.h"
#include "BitStream.h"
#include "Log.h"
#include "CommonFlag.h"
#include "NetMsg.h"

using namespace hudp;

bool CSerializes::Serializes(NetMsg& msg, CHudpBitStream& stream) {
    return Serializes(msg._head, msg._body, msg._head._body_len, stream);
}

bool CSerializes::Serializes(const Head& head, CHudpBitStream& stream) {
    // must serializes head first
    if (stream.GetCurrentLength() > 0) {
        base::LOG_ERROR("head is not first to be serializes");
        return false;
    }
    
    // fixed sequence by read/write
    stream.Write(head._flag);
    if (head._flag & HPF_WITH_ID) {
        CHECK_RET(stream.Write(head._id));
    }
    if (head._flag & HPF_WITH_ACK_RANGE) {
        CHECK_RET(stream.Write(head._ack_len));
        CHECK_RET(stream.Write(head._ack_start));
    }
    if (head._flag & HPF_WITH_ACK_ARRAY) {
        CHECK_RET(stream.Write(head._ack_len));
        for(auto iter = head._ack_vec.begin(); iter != head._ack_vec.end(); ++iter) {
            CHECK_RET(stream.Write(*iter));
        }
    }
    if (head._flag & HPF_WITH_BODY) {
        CHECK_RET(stream.Write(head._body_len));
    }
    return true;
}

bool CSerializes::Serializes(Head& head, const char* body, uint16_t len, CHudpBitStream& stream) {
    if (len > 0) {
        head._body_len = len;
        head._flag |= HPF_WITH_BODY;
    }
    if (!Serializes(head, stream)) {
        return false;
    }
    
    return stream.Write(body, len);
}

bool CSerializes::Deseriali(CHudpBitStream& stream, NetMsg& msg) {
    return Deseriali(stream, msg._head, msg._body, msg._head._body_len);
}

bool CSerializes::Deseriali(CHudpBitStream& stream, Head& head) {
    
    // fixed sequence by read/write
    stream.Read(head._flag);
    if (head._flag & HPF_WITH_ID) {
        CHECK_RET(stream.Read(head._id));
    }
    if (head._flag & HPF_WITH_ACK_RANGE) {
        CHECK_RET(stream.Read(head._ack_len));
        CHECK_RET(stream.Read(head._ack_start));
    }
    if (head._flag & HPF_WITH_ACK_ARRAY) {
        CHECK_RET(stream.Read(head._ack_len));
        for (auto iter = head._ack_vec.begin(); iter != head._ack_vec.end(); ++iter) {
            CHECK_RET(stream.Read(*iter));
        }
    }
    if (head._flag & HPF_WITH_BODY) {
        CHECK_RET(stream.Read(head._body_len));
    }
}
  
bool CSerializes::Deseriali(CHudpBitStream& stream, Head& head, char* body, uint16_t& len) {
    if (!Deseriali(stream, head)) {
        return false;
    }
    CHECK_RET(stream.Read(body, head._body_len));
    len = head._body_len;
    return true;
}