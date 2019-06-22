#ifndef HEADER_CBITSTREAM
#define HEADER_CBITSTREAM

#include "Serializes.h"
#include "BitStream.h"
#include "Log.h"

namespace hudp {

#define CEHCK_RET(ret) do{if(!ret) return false;}while(0);

    bool Serializes(NetMsg& msg, CHudpBitStream& stream) {
        return Serializes(msg._head, msg._body, msg._head._body_len, stream);
    }

    bool Serializes(const Head& head, CHudpBitStream& stream) {
        // must serializes head first
        if (stream.GetCurrentLength() > 0) {
            base::LOG_ERROR("head is not first to be serializes");
            return false;
        }
        
        // fixed sequence by read/write
        stream.Write(head._flag);
        if (head._flag & HPF_WITH_ID) {
            CEHCK_RET(stream.Write(head._id));
        }
        if (head._flag & HPF_WITH_ACK_RANGE) {
            CEHCK_RET(stream.Write(head._ack_len));
            CEHCK_RET(stream.Write(head._ack_start));
        }
        if (head._flag & HPF_WITH_ACK_ARRAY) {
            CEHCK_RET(stream.Write(head._ack_len));
            for(auto iter = head._ack_vec.begin(); iter != head._ack_vec.end(); ++iter) {
                CEHCK_RET(stream.Write(*iter));
            }
        }
        if (head._flag & HPF_WITH_BODY) {
            CEHCK_RET(stream.Write(head._body_len));
        }
        return true;
    }

    bool Serializes(Head& head, const char* body, uint16_t len, CHudpBitStream& stream) {
        if (len > 0) {
            head._body_len = len;
            head._flag |= HPF_WITH_BODY;
        }
        if (!Serializes(head, stream)) {
            return false;
        }
        
        return stream.Write(body, len);
    }

    bool Deseriali(CHudpBitStream& stream, NetMsg& msg) {
        return Deseriali(stream, msg._head, msg._body, msg._head._body_len);
    }

    bool Deseriali(CHudpBitStream& stream, Head& head) {
        
        // fixed sequence by read/write
        stream.Read(head._flag);
        if (head._flag & HPF_WITH_ID) {
            CEHCK_RET(stream.Read(head._id));
        }
        if (head._flag & HPF_WITH_ACK_RANGE) {
            CEHCK_RET(stream.Read(head._ack_len));
            CEHCK_RET(stream.Read(head._ack_start));
        }
        if (head._flag & HPF_WITH_ACK_ARRAY) {
            CEHCK_RET(stream.Read(head._ack_len));
            for (auto iter = head._ack_vec.begin(); iter != head._ack_vec.end(); ++iter) {
                CEHCK_RET(stream.Read(*iter));
            }
        }
        if (head._flag & HPF_WITH_BODY) {
            CEHCK_RET(stream.Read(head._body_len));
        }
    }
      
    bool Deseriali(CHudpBitStream& stream, Head& head, char* body, uint16_t& len) {
        if (!Deseriali(stream, head)) {
            return false;
        }
        CEHCK_RET(stream.Read(body, head._body_len));
        len = head._body_len;
        return true;
    }
}

#endif