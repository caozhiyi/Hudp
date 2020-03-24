#include "Log.h"
#include "MsgImpl.h"
#include "MsgHead.h"
#include "BitStream.h"
#include "CommonFlag.h"
#include "Serializes.h"

using namespace hudp;

bool CSerializesNormal::Serializes(CMsg& msg, CBitStreamWriter& bit_stream) {
    return Serializes(msg.GetHead(), msg.GetBody().c_str(), (uint16_t)msg.GetBody().length(), bit_stream);
}

bool CSerializesNormal::Deseriali(CBitStreamReader& bit_stream, CMsg& msg) {
    return Deseriali(bit_stream, msg.GetHead(), msg.GetBody());
}

uint32_t CSerializesNormal::EstimateSize(CMsg& msg) {
    uint32_t ret = 0;

    auto& head = msg.GetHead();
    uint32_t header_flag = head.GetFlag();

    ret += sizeof(header_flag);
    if (header_flag & HPF_WITH_ID) {
        ret += sizeof(uint16_t);
    }

    if (header_flag & HPF_MSG_WITH_TIME_STAMP) {
        ret += sizeof(uint64_t);
    }

    if (header_flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        std::vector<uint16_t> reliable_orderly_vec;
        head.GetReliableOrderlyAck(reliable_orderly_vec);
        uint16_t len = (uint16_t)reliable_orderly_vec.size();
        if (len > 0) {
            ret += sizeof(uint16_t);
            if (header_flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {
                ret += sizeof(uint16_t);

            } else {
                ret += sizeof(uint16_t) * len;
            }
        }
        if (header_flag & HPF_MSG_WITH_TIME_STAMP) {
            std::vector<uint64_t> time_vec;
            head.GetReliableOrderlyAckTime(time_vec);
            ret += sizeof(uint64_t) * len;
        }
    }

    if (header_flag & HPF_WITH_RELIABLE_ACK) {
        std::vector<uint16_t> reliable_vec;
        head.GetReliableAck(reliable_vec);
        uint16_t len = (uint16_t)reliable_vec.size();
        if (len > 0) {
            ret += sizeof(uint16_t);
            if (header_flag & HPF_RELIABLE_ACK_RANGE) {
                ret += sizeof(uint16_t);

            } else {
                ret += sizeof(uint16_t) * len;
            }
        }
        if (header_flag & HPF_MSG_WITH_TIME_STAMP) {
            std::vector<uint64_t> time_vec;
            head.GetReliableAckTime(time_vec);
            ret += sizeof(uint64_t) * len;
        }
    }
    
    if (header_flag & HPF_WITH_BODY) {
        ret += head.GetBodyLength();
    }
    return ret;
}

bool CSerializesNormal::SerializesHead(Head& head, CBitStreamWriter& bit_stream) {
    // must serializes head first
    if (bit_stream.GetCurrentLength() > 0) {
        base::LOG_ERROR("head is not first to be serializes");
        return false;
    }
    
    // fixed sequence by read/write
    uint32_t flag = head.GetFlag();
    bit_stream.Write(flag);
    if (flag & HPF_WITH_ID) {
        CHECK_RET(bit_stream.Write(head.GetId()));
    }

    if (flag & HPF_MSG_WITH_TIME_STAMP) {
        CHECK_RET(bit_stream.Write(head.GetSendTime()));
    }

    if (flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        std::vector<uint16_t> reliable_orderly_vec;
        head.GetReliableOrderlyAck(reliable_orderly_vec);
        uint16_t len = (uint16_t)reliable_orderly_vec.size();
        if (len > 0) {
            CHECK_RET(bit_stream.Write(len));
            if (flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {
                CHECK_RET(bit_stream.Write(reliable_orderly_vec[0]));

            } else {
                for (uint16_t i = 0; i < len; i++) {
                    CHECK_RET(bit_stream.Write(reliable_orderly_vec[i]));
                }
            }
        }
        if (flag & HPF_MSG_WITH_TIME_STAMP) {
            std::vector<uint64_t> time_vec;
            head.GetReliableOrderlyAckTime(time_vec);
            for (uint16_t i = 0; i < len; i++) {
                CHECK_RET(bit_stream.Write(time_vec[i]));
            }
        }
    }

    if (flag & HPF_WITH_RELIABLE_ACK) {
        std::vector<uint16_t> reliable_vec;
        head.GetReliableAck(reliable_vec);
        uint16_t len = (uint16_t)reliable_vec.size();
        if (len > 0) {
            CHECK_RET(bit_stream.Write(len));
            if (flag & HPF_RELIABLE_ACK_RANGE) {
                CHECK_RET(bit_stream.Write(reliable_vec[0]));

            } else {
                for (uint16_t i = 0; i < len; i++) {
                    CHECK_RET(bit_stream.Write(reliable_vec[i]));
                }
            }
        }
        if (flag & HPF_MSG_WITH_TIME_STAMP) {
            std::vector<uint64_t> time_vec;
            head.GetReliableAckTime(time_vec);
            for (uint16_t i = 0; i < len; i++) {
                CHECK_RET(bit_stream.Write(time_vec[i]));
            }
        }
    }
    
    if (flag & HPF_WITH_BODY) {
        CHECK_RET(bit_stream.Write(head.GetBodyLength()));
    }
    return true;
}

bool CSerializesNormal::Serializes(Head& head, const char* body, uint16_t len, CBitStreamWriter& bit_stream) {
    if (len > 0) {
        head.SetBodyLength(len);
    }
    if (!SerializesHead(head, bit_stream)) {
        return false;
    }
    
    return bit_stream.Write(body, len);
}

bool CSerializesNormal::DeserialiHead(CBitStreamReader& bit_stream, Head& head) {
    // fixed sequence by read/write
    uint32_t flag = 0;
    bit_stream.Read(flag);
    head.SetFlag(flag);
    if (flag & HPF_WITH_ID) {
        uint16_t id;
        CHECK_RET(bit_stream.Read(id));
        head.SetId(id);
    }

    if (flag & HPF_MSG_WITH_TIME_STAMP) {
        uint64_t time_stamp = 0;
        CHECK_RET(bit_stream.Read(time_stamp));
        head.SetSendTime(time_stamp);
    }

    if (flag & HPF_WITH_RELIABLE_ORDERLY_ACK) {
        uint16_t len = 0;
        CHECK_RET(bit_stream.Read(len));
        uint16_t tmp = 0;
        if (flag & HPF_RELIABLE_ORDERLY_ACK_RANGE) {
            CHECK_RET(bit_stream.Read(tmp));
            std::vector<uint16_t> ack_vec;
            for (uint16_t i = 0; i < len; i++) {
                ack_vec.push_back(tmp + i);
            }
            head.AddReliableOrderlyAck(ack_vec, true);

        } else {
            std::vector<uint16_t> ack_vec;
            for (uint16_t i = 0; i < len; i++) {
                CHECK_RET(bit_stream.Read(tmp));
                ack_vec.push_back(tmp);
            }
            head.AddReliableOrderlyAck(ack_vec);
        }

        if (flag & HPF_MSG_WITH_TIME_STAMP) {
            std::vector<uint64_t> time_vec;
            uint64_t time_tmp = 0;
            for (uint16_t i = 0; i < len; i++) {
                CHECK_RET(bit_stream.Read(time_tmp));
                time_vec.push_back(time_tmp);
            }
            head.AddReliableOrderlyAckTime(time_vec);
        }
    }
    if (flag & HPF_WITH_RELIABLE_ACK) {
        uint16_t len = 0;
        CHECK_RET(bit_stream.Read(len));
        uint16_t tmp = 0;
        if (flag & HPF_RELIABLE_ACK_RANGE) {
            CHECK_RET(bit_stream.Read(tmp));
            std::vector<uint16_t> ack_vec;
            for (uint16_t i = 0; i < len; i++) {
                ack_vec.push_back(tmp + i);
            }
             head.AddReliableAck(ack_vec, true);

        } else {
            std::vector<uint16_t> ack_vec;
            for (uint16_t i = 0; i < len; i++) {
                CHECK_RET(bit_stream.Read(tmp));
                ack_vec.push_back(tmp);
            }
            head.AddReliableAck(ack_vec);
        }

        if (flag & HPF_MSG_WITH_TIME_STAMP) {
            std::vector<uint64_t> time_vec;
            uint64_t time_tmp = 0;
            for (uint16_t i = 0; i < len; i++) {
                CHECK_RET(bit_stream.Read(time_tmp));
                time_vec.push_back(time_tmp);
            }
            head.AddReliableAckTime(time_vec);
        }
    }

    if (flag & HPF_WITH_BODY) {
        uint16_t body_len = 0;
        CHECK_RET(bit_stream.Read(body_len));
        head.SetBodyLength(body_len);
    }
    return true;
}

bool CSerializesNormal::Deseriali(CBitStreamReader& bit_stream, Head& head, std::string& body) {
    if (!DeserialiHead(bit_stream, head)) {
        return false;
    }
    return bit_stream.Read(body, head.GetBodyLength());
}
