#ifndef HEADER_INTERFACE_FILTER
#define HEADER_INTERFACE_FILTER

#include <memory> // for shared_ptr

namespace hudp {
    // msg filter base.
    // If you want to add a inclusion process, can inherit this interface.
    class CMsg;
    class CFilter
    {
    public:
        CFilter() {}
        virtual ~CFilter() {}
        // when send msg filter process
        virtual void FilterProcess(std::shared_ptr<CMsg> msg) = 0;
        // when recv msg filter process
        virtual void RelieveFilterProcess(std::shared_ptr<CMsg> msg) = 0;
    };
}
#endif