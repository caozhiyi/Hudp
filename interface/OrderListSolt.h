#ifndef HEADER_INTERFACE_ORDERLISTSOLT
#define HEADER_INTERFACE_ORDERLISTSOLT

namespace hudp {
    class CMsg;
    class COrderListSolt
    {
    public:
        COrderListSolt() {}
        virtual ~COrderListSolt() {}

        virtual void ToRecv(CMsg* msg) = 0;
    };
}
#endif