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
        bool GetAllAck(std::vector<uint16_t>& ack_vec);

        // get consecutive ack id. only clear consecutive id cache.
        bool GetConsecutiveAck(uint16_t& start, uint16_t& len);

    private:
        // check id in set is consecutive?
        void CheckConsecutive();

    private:
        std::mutex  _mutex;
        // consecutive ack id length
        uint16_t    _ack_len;
        // start id of consecutive ack
        uint16_t    _ack_start;
        // expect start id
        uint16_t   _expect_start_id;
        // all ack id cache
        std::set<uint16_t> _ack_set;
    };
}

#endif