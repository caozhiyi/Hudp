#ifndef HEADER_INTERFACE_ORDERLIST
#define HEADER_INTERFACE_ORDERLIST

#include <memory> // for shared_ptr
#include "CommonType.h"

namespace hudp {
    class CMsg;
    class COrderList
    {
    public:
        COrderList() {}
        virtual ~COrderList() {}

        // add a item to order list
        virtual uint16_t Insert(std::shared_ptr<CMsg> msg) = 0;

        // make id little than order list max size
        virtual uint16_t HashFunc(uint16_t id) = 0;
    };
}
#endif