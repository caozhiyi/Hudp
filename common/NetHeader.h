#ifndef HEADER_COMMON_NETMSGHEADER
#define HEADER_COMMON_NETMSGHEADER
#include <vector>
#include <string>

#include "CommonType.h"
#include "CommonFlag.h"
#include "HudpConfig.h"

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

        // body length
        void     SetBodyLength(uint16_t length);
        uint16_t GetBodyLength();

        // id
        void     SetId(uint16_t id);
        uint16_t GetId();

        // reliable orderly ack
        // only called once 
        bool AddReliableOrderlyAck(const std::vector<uint16_t>& ack_vec, bool continue_range = false);
        bool GetReliableOrderlyAck(std::vector<uint16_t>& ack_vec);

        // reliable ack
        // only called once 
        bool AddReliableAck(const std::vector<uint16_t>& ack_vec, bool continue_range = false);
        bool GetReliableAck(std::vector<uint16_t>& ack_vec);
    private:
        friend class CSerializes;
        uint16_t _flag;
        uint16_t _id;
        uint16_t _body_len;
 
        // reliable ack
        uint16_t _ack_reliable_len;
        std::vector<uint16_t> _ack_reliable_vec;

        // reliable orderly ack
        uint16_t _ack_reliable_orderly_len;
        std::vector<uint16_t> _ack_reliable_orderly_vec;
    };
}
#endif
