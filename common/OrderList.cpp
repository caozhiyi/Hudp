#include <cstring>		//for memset
#include "OrderList.h"
using namespace hudp;

CRecvList::CRecvList() : _expect_id(1) {
    memset(_order_list, 0, sizeof(_order_list));
}

CRecvList::~CRecvList() {

}

uint16_t CRecvList::HashFunc(uint16_t id) {
    return id & (__order_list_size - 1);
}

uint16_t CReliableOrderlyList::Insert(uint16_t id, COrderListSolt* ol) {
    uint16_t index = HashFunc(id);
    
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (id == _expect_id) {
            _expect_id++;
            _order_list[index] = ol;
            while (_order_list[index]) {
                _recv_list.Push(_order_list[index]);
                _order_list[index] = nullptr;

                index++;
                if (index >= __order_list_size) {
                    index = 0;
                }
            }

        // is't expect id
        } else {
            // a repeat bag
            if (_order_list[index]) {
                return 1;
            } else {
                _order_list[index] = ol;
            }
            
        }
    }
    
    if (_recv_list.Size() > 0) {
        COrderListSolt* item = nullptr;

        while (_recv_list.Pop(item)) {
            item->ToRecv();
        }
        _recv_list.Clear();
    }
    return 0;
}
   
uint16_t CReliableList::Insert(uint16_t id, COrderListSolt* ol) {
    // msg repeat TO DO
    ol->ToRecv();
    return 0;
}
    
uint16_t COrderlyList::Insert(uint16_t id, COrderListSolt* ol) {
    
    if (id < _expect_id) {
        return 0;
    }

    _expect_id = id;
    ol->ToRecv();
    return 0;
}