#ifndef HEADER_FILTER_HEAD_FILTER
#define HEADER_FILTER_HEAD_FILTER

#include "IFilter.h"

namespace hudp {
    // the firt filter
    // send msg to upper
    class CHeadFilter : public CFilter {
    public:
        CHeadFilter() {}
        virtual ~CHeadFilter() {}
        // when send msg filter process
        bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
        // when recv msg filter process
        bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
    };
}
#endif