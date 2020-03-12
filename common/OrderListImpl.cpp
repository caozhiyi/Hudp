#include <cstring>		//for memset

#include "IMsg.h"
#include "ISocket.h"
#include "HudpImpl.h"
#include "HudpConfig.h"
#include "OrderListImpl.h"

using namespace hudp;

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
    std::unique_lock<std::mutex> lock(_mutex);
    for (size_t i = 0; i < __order_list_size; i++) {
        if (_order_list[i]) {
            // return to msg pool
            CHudpImpl::Instance().ReleaseMessage(_order_list[i]);
        }
    }
}

uint16_t CReliableOrderlyList::Insert(CMsg* msg) {
	auto id = msg->GetId();
    uint16_t index = HashFunc(id);
    
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (id == _expect_id) {
			_order_list[index] = msg;
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
				_order_list[index] = msg;
            }
            
        }
    }
    
    if (_recv_list.Size() > 0) {
		CMsg* item = nullptr;

        while (_recv_list.Pop(item)) {
			auto sock = item->GetSocket();
			sock->ToRecv(item);
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
   
uint16_t CReliableList::Insert(CMsg* msg) {
	auto id = msg->GetId();
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
	auto sock = msg->GetSocket();
	sock->ToRecv(msg);
    return 0;
}

COrderlyList::COrderlyList() : _expect_id(0) {

}

COrderlyList::~COrderlyList() {

}

uint16_t COrderlyList::Insert(CMsg* msg) {
	auto id = msg->GetId();
    if (id < _expect_id) {
        return 0;
    }

    _expect_id = id;
	auto sock = msg->GetSocket();
    sock->ToRecv(msg);
    return 0;
}
