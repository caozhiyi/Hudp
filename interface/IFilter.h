#ifndef HEADER_INTERFACE_FILTER
#define HEADER_INTERFACE_FILTER

#include <memory> // for shared_ptr
#include "HudpFlag.h"

namespace hudp {
    // msg filter base.
    // if you want to add a message filter, can inherit this interface.
    // add filter in CHudpImpl::Init().
    class CFilter {
    public:
        CFilter() : _next_filter(nullptr), _prev_filter(nullptr) {}
        virtual ~CFilter() { _next_filter.reset(); _prev_filter.reset(); }
        // when send msg filter process
        virtual bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id) = 0;
        // when recv msg filter process
        virtual bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) = 0;
        // two way linked list about
        void SetNextFilter(std::shared_ptr<CFilter> filter) { _next_filter = filter; }
        void SetPrevFilter(std::shared_ptr<CFilter> filter) { _prev_filter = filter; }
        std::shared_ptr<CFilter> GetNextFilter() { return _next_filter; }
        std::shared_ptr<CFilter> GetPrevFilter() { return _prev_filter; }
    protected:
        // two way linked list about
        std::shared_ptr<CFilter> _next_filter;
        std::shared_ptr<CFilter> _prev_filter;
    };
}
#endif