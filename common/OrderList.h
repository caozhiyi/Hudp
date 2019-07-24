#ifndef HEADER_COMMON_ORDERLIST
#define HEADER_COMMON_ORDERLIST

#include <mutex>
#include "OrderListSolt.h"
#include "CommonType.h"
#include "TSQueue.h"

namespace hudp {
    
    static const uint16_t __order_list_size = 256; // 2 >> 8

    // order list base class
    class CRecvList {
    public:
        CRecvList();
        virtual ~CRecvList();

        // add a item to order list
        virtual uint16_t Insert(uint16_t id, COrderListSolt* ol) = 0;

        // make id little than order list size
        uint16_t HashFunc(uint16_t id);

    protected:
        uint16_t _expect_id;   
    
        COrderListSolt* _order_list[__order_list_size];
        base::CTSQueue<COrderListSolt*> _recv_list;
    };

    // receive list that reliable and orderly 
    class CReliableOrderlyList : public CRecvList {
    public:
        CReliableOrderlyList() {}
        virtual ~CReliableOrderlyList() {}
        // add a item to order list
        // return 0 if success
        // return 1 if msg is repeat
        virtual uint16_t Insert(uint16_t id, COrderListSolt* ol);
    private:
        std::mutex _mutex;
    };

    //  receive list that only reliable
    class CReliableList : public CRecvList {
    public:
        CReliableList() {}
        virtual ~CReliableList() {}
        // add a item to order list
        // return 0 if success
        // return 1 if msg is repeat
        virtual uint16_t Insert(uint16_t id, COrderListSolt* ol);
    };

    // receive list that only orderly
    class COrderlyList : public CRecvList {
    public:
        COrderlyList() {}
        virtual ~COrderlyList() {}
        // add a item to order list
        // always return 0
        virtual uint16_t Insert(uint16_t id, COrderListSolt* ol);
    };
}
#endif