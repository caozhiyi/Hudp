#ifndef HEADER_COMMON_PENDACK
#define HEADER_COMMON_PENDACK

#include <map>
#include <mutex>
#include <vector>

#include "CommonType.h"

namespace hudp {

    class CPendAck {
    public:
        CPendAck();
        ~CPendAck();
        void AddAck(uint16_t ack_id);
        void AddAck(uint16_t ack_id, uint64_t send_time);

        // clear all msg
        void Clear();

        // get ack under size
        bool GetAck(std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool& continuity, uint16_t size_limit);

        // get all ack and clear cache
        bool GetAllAck(std::vector<uint16_t>& ack_vec, bool& continuity);
        bool GetAllAck(std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool& continuity);

        bool HasAck();

        void SetSendAckNowCallBack(std::function<void()> cb);

    private:
        std::mutex  _mutex;
        // all ack id cache
        uint32_t    _cur_size;
        std::map<uint16_t, uint64_t> _ack_map;
        std::function<void()>        _ack_now_call_back;
    };
}

#endif