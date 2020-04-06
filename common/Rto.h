#ifndef HEADER_COMMON_RTO
#define HEADER_COMMON_RTO

#include <mutex>
#include <unordered_map>

#include "IRto.h"
#include "CommonType.h"

namespace hudp {

    // Calculating RTO with standard method and timestamp,
    // is the same as Linux method.
    // rto = srtt + 4 * rttvar;
    class CRtoImpl : public CRto {
    public:
        CRtoImpl();
        ~CRtoImpl();
        // set rtt time
        void SetRttTime(uint64_t rtt);
        // get rto time
        uint32_t GetRto();
    private:
        std::mutex  _mutex;
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