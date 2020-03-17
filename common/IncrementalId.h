#ifndef HEADER_COMMON_INCREAMENTID
#define HEADER_COMMON_INCREAMENTID

#include <mutex>
#include "CommonType.h"

namespace hudp {

    class CIncrementalId {
    public:
        CIncrementalId(uint16_t cur_id);
        ~CIncrementalId();
        
        // get next incremental id
        uint16_t GetNextId();

        static bool IsNextId(uint16_t prev, uint16_t next);
        static bool IsSmallerThan(const uint16_t& prev, const uint16_t& next);
        static bool IsBiggerThan(const uint16_t& prev, const uint16_t& next);

    private:
        std::mutex  _mutex;
        uint16_t    _cur_id;
    };

}
#endif
