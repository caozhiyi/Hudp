#include <cstring>		//for memset
#include "OrderList.h"
using namespace hudp;

static const uint16_t __msx_cache_msg_num = 100;

CRecvList::CRecvList() {
    
}

CRecvList::~CRecvList() {

}

uint16_t CRecvList::HashFunc(uint16_t id) {
    return id & (__order_list_size - 1);
}

CReliableOrderlyList::CReliableOrderlyList() : _expect_id(1) {
    memset(_order_list, 0, sizeof(_order_list));
}

CReliableOrderlyList::~CReliableOrderlyList() {

}

uint16_t CReliableOrderlyList::Insert(uint16_t id, COrderListSolt* ol) {
    uint16_t index = HashFunc(id);
    
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (id == _expect_id) {
            _order_list[index] = ol;
            while (_order_list[index]) {
                _expect_id++;
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

CReliableList::CReliableList() : _msg_num(0), _start(0) {
    memset(_order_list, 0, sizeof(_order_list));
}

CReliableList::~CReliableList() {

}
   
uint16_t CReliableList::Insert(uint16_t id, COrderListSolt* ol) {
    uint16_t index = HashFunc(id);
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (_order_list[index] != 0) {
            return 1;

        } else {
            _order_list[index] = id;
            _msg_num++;

            // clear cache
            if (__msx_cache_msg_num < _msg_num) {
                _order_list[_start] = 0;
                _start++;
                if (_start >= __order_list_size) {
                    _start = 0;
                }
            }
        }
    }
    ol->ToRecv();
    return 0;
}

COrderlyList::COrderlyList() : _expect_id(0) {

}

COrderlyList::~COrderlyList() {

}

uint16_t COrderlyList::Insert(uint16_t id, COrderListSolt* ol) {
    
    if (id < _expect_id) {
        return 0;
    }

    _expect_id = id;
    ol->ToRecv();
    return 0;
}