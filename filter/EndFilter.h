#ifndef HEADER_FILTER_END_FILTER
#define HEADER_FILTER_END_FILTER

#include "IFilter.h"

namespace hudp {
    // the last filter
    // send msg to hudp
    class CEndFilter : public CFilter {
    public:
        CEndFilter() {}
        virtual ~CEndFilter() {}
        // when send msg filter process
        bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
        // when recv msg filter process
        bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
    };
}
#endif