#ifndef HEADER_COMMON_PENDACK
#define HEADER_COMMON_PENDACK

#include <set>
#include <vector>
#include <mutex>

#include "CommonType.h"

namespace hudp {

    class CPendAck {
    public:
        CPendAck();
        ~CPendAck();
        void AddAck(uint16_t ack_id);

        // get all ack and clear cache
        bool GetAllAck(std::vector<uint16_t>& ack_vec, bool& continuity);

        bool HasAck();

    private:
        std::mutex  _mutex;
        // all ack id cache
        std::set<uint16_t> _ack_set;
    };
}

#endif