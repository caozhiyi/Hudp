#ifndef HEADER_FILTER_HEAD_FILTER
#define HEADER_FILTER_HEAD_FILTER

#include "IFilter.h"

namespace hudp {
    // the first filter
    // send msg to upper
    class CHeadFilter : public CFilter {
    public:
        CHeadFilter() {}
        virtual ~CHeadFilter() {}
        // when send msg filter process
        bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id);
        // when recv msg filter process
        bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
    };
}
#endif
