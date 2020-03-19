#ifndef HEADER_COMMON_NETMSGHEADER
#define HEADER_COMMON_NETMSGHEADER

#include <vector>
#include "CommonType.h"

namespace hudp {

    // net message header, not thread safe.
    class Head {
    public:
        Head();
        ~Head();
        // clear all member 
        void Clear();

        // clear member which about ack
        void ClearAck();

        void SetFlag(uint32_t flag);
        uint32_t GetFlag();

        // body length
        void     SetBodyLength(uint16_t length);
        uint16_t GetBodyLength();

        // id
        void     SetId(uint16_t id);
        uint16_t GetId();

        // send time
        void SetSendTime(uint64_t time);
        uint64_t GetSendTime();

        // reliable orderly ack
        // only called once 
        bool AddReliableOrderlyAck(const std::vector<uint16_t>& ack_vec, bool continue_range = false);
        bool GetReliableOrderlyAck(std::vector<uint16_t>& ack_vec);

        bool AddReliableOrderlyAckTime(const std::vector<uint64_t>& time_vec);
        bool GetReliableOrderlyAckTime(std::vector<uint64_t>& time_vec);

        // reliable ack
        // only called once 
        bool AddReliableAck(const std::vector<uint16_t>& ack_vec, bool continue_range = false);
        bool GetReliableAck(std::vector<uint16_t>& ack_vec);

        bool AddReliableAckTime(const std::vector<uint64_t>& time_vec);
        bool GetReliableAckTime(std::vector<uint64_t>& time_vec);

    private:
        friend class CMsgImpl;
        uint32_t _flag;     // this flag  will be send to remote
        uint16_t _id;       // msg id
        uint16_t _body_len; // body length if have

        uint64_t _send_time;// time stamp 
 
        // reliable ack
        std::vector<uint16_t> _ack_reliable_vec;
        // msg send time, ack use
        std::vector<uint64_t> _reliable_msg_send_time_vec;

        // reliable orderly ack
        std::vector<uint16_t> _ack_reliable_orderly_vec;
        // msg send time, ack use
        std::vector<uint64_t> _reliable_orderly_msg_send_time_vec;

  
    };
}
#endif
