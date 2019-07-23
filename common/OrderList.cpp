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

void CReliableOrderlyList::Insert(uint16_t id, COrderListSolt* ol) {
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
                _recv_list.Push(ol);
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
}
   
void CReliableList::Insert(uint16_t id, COrderListSolt* ol) {
    ol->ToRecv();
}
    
void COrderlyList::Insert(uint16_t id, COrderListSolt* ol) {
    
    if (id < _expect_id) {
        return;
    }

    _expect_id = id;
    ol->ToRecv();
}