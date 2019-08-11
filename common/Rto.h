#ifndef HEADER_COMMON_RTO
#define HEADER_COMMON_RTO

#include <mutex>
#include <unordered_map>
#include "CommonType.h"

namespace hudp {

    // Calculating RTO with standard method and timestamp,
    // is the same as Linux method.
    // rto = srtt + 4 * rttvar;
    class CRto {
    public:
        CRto();
        ~CRto();
        // set send time.
        void SetIdTime(uint16_t id, uint64_t time);
        // remove item from map
        void RemoveIdTime(uint16_t id);
        // set ack time
        void SetAckTime(uint16_t id, uint64_t time);
        // get rto
        uint32_t GetRto();
    private:
        std::mutex  _mutex;
        // id <=> send time
        std::unordered_map<uint16_t, uint64_t> _id_time;
        // mean estimation
        uint32_t    _srtt;
        // estimation of absolute error
        uint32_t    _mdev;
        // max mdev
        uint32_t    _mdev_max;
        // rto
        uint32_t    _cur_rto;
    };
}

#endif