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

        // get all ack and clear cache
        bool GetAllAck(std::vector<uint16_t>& ack_vec, bool& continuity);
        bool GetAllAck(std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool& continuity);

        bool HasAck();

    private:
        std::mutex  _mutex;
        // all ack id cache
        uint16_t    _start;
        std::map<uint16_t, uint64_t> _ack_set;
    };
}

#endif