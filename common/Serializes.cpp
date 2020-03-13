#include "Log.h"
#include "MsgImpl.h"
#include "MsgHead.h"
#include "BitStream.h"
#include "CommonFlag.h"
#include "Serializes.h"

using namespace hudp;

bool CSerializes::Serializes(CMsgImpl& msg, CBitStreamWriter& bit_stream) {
    return Serializes(msg._head, msg._body.c_str(), msg._body.length(), bit_stream);
}

bool CSerializes::Serializes(const Head& head, CBitStreamWriter& bit_stream) {
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

    if (head._flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        if (head._ack_reliable_orderly_len > 0) {
            CHECK_RET(bit_stream.Write(head._ack_reliable_orderly_len));
            if (head._flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {
                CHECK_RET(bit_stream.Write(head._ack_reliable_orderly_vec[0]));

            } else {
                for (uint16_t i = 0; i < head._ack_reliable_orderly_len; i++) {
                    CHECK_RET(bit_stream.Write(head._ack_reliable_orderly_vec[i]));
                }
            }
        }
    }
    if (head._flag & HPF_WITH_RELIABLE_ACK) {
        if (head._ack_reliable_len > 0) {
            CHECK_RET(bit_stream.Write(head._ack_reliable_len));
            if (head._flag & HPF_RELIABLE_ACK_RANGE) {
                CHECK_RET(bit_stream.Write(head._ack_reliable_vec[head._ack_reliable_orderly_len]));

            } else {
                for (uint16_t i = head._ack_reliable_orderly_len; i < head._ack_reliable_len; i++) {
                    CHECK_RET(bit_stream.Write(head._ack_reliable_vec[i]));
                }
            }
        }
    }
    
    if (head._flag & HPF_WITH_BODY) {
        CHECK_RET(bit_stream.Write(head._body_len));
    }
    return true;
}

bool CSerializes::Serializes(Head& head, const char* body, uint16_t len, CBitStreamWriter& bit_stream) {
    if (len > 0) {
        head._body_len = len;
    }
    if (!Serializes(head, bit_stream)) {
        return false;
    }
    
    return bit_stream.Write(body, len);
}

bool CSerializes::Deseriali(CBitStreamReader& bit_stream, CMsgImpl& msg) {
    return Deseriali(bit_stream, msg._head, msg._body);
}

bool CSerializes::Deseriali(CBitStreamReader& bit_stream, Head& head) {
    // fixed sequence by read/write
    bit_stream.Read(head._flag);
    if (head._flag & HPF_WITH_ID) {
        CHECK_RET(bit_stream.Read(head._id));
    }

    if (head._flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        CHECK_RET(bit_stream.Read(head._ack_reliable_orderly_len));
        uint16_t tmp = 0;
        if (head._flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {
            CHECK_RET(bit_stream.Read(tmp));
            head._ack_reliable_orderly_vec.push_back(tmp);

        } else {
            for (uint16_t i = 0; i < head._ack_reliable_orderly_len; i++) {
                CHECK_RET(bit_stream.Read(tmp));
                head._ack_reliable_orderly_vec.push_back(tmp);
            }
        }
    }
    if (head._flag & HPF_WITH_RELIABLE_ACK) {
        CHECK_RET(bit_stream.Read(head._ack_reliable_len));
        uint16_t tmp = 0;
        if (head._flag & HPF_RELIABLE_ACK_RANGE) {
            CHECK_RET(bit_stream.Read(tmp));
            head._ack_reliable_vec.push_back(tmp);

        } else {
            for (uint16_t i = head._ack_reliable_orderly_len; i < head._ack_reliable_len; i++) {
                CHECK_RET(bit_stream.Read(tmp));
                head._ack_reliable_vec.push_back(tmp);
            }
        }
    }

    if (head._flag & HPF_WITH_BODY) {
        CHECK_RET(bit_stream.Read(head._body_len));
    }
    return true;
}
  
bool CSerializes::Deseriali(CBitStreamReader& bit_stream, Head& head, std::string& body) {
    if (!Deseriali(bit_stream, head)) {
        return false;
    }
    CHECK_RET(bit_stream.Read(body, head._body_len));
    return true;
}