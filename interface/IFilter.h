#ifndef HEADER_INTERFACE_FILTER
#define HEADER_INTERFACE_FILTER

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
        virtual void FilterProcess(CMsg* msg) = 0;
        // when recv msg filter process
        virtual void RelieveFilterProcess(CMsg* msg) = 0;
    };
}
#endif