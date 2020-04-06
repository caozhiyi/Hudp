#ifndef HEADER_INTERFACE_RTO
#define HEADER_INTERFACE_RTO

#include "CommonType.h"

namespace hudp {
    // rto calc base class.
    class CRto {
    public:
        CRto() {}
        virtual ~CRto() {}
        // set rtt time
        virtual void SetRttTime(uint64_t rtt) = 0;
        // get rto
        virtual uint32_t GetRto() = 0;
    };
}
#endif