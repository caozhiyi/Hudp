#include <cstring>		//for memset
#include "Socket.h"
#include "SendWnd.h"
#include "OrderList.h"
#include "PriorityQueue.h"
#include "IncrementalId.h"
#include "NetMsg.h"
#include "Log.h"

using namespace hudp;

CSocket::CSocket(const std::string& ip, uint16_t port, uint64_t sock) : _short(port), _socket(sock), _pri_queue(new CPriorityQueue) {
    memcpy(_ip, ip.c_str(), ip.length());
    memset(_inc_id, 0, sizeof(_inc_id));
    memset(_send_wnd, 0, sizeof(_send_wnd));
    memset(_recv_list, 0, sizeof(_recv_list));
}

CSocket::~CSocket() {
    for (uint16_t i = 0; i < __wnd_size; i++) {
        if (_inc_id[i]) {
            delete _inc_id[i];
        }
        if (_send_wnd[i]) {
            
            delete _send_wnd[i];
        }
        if (_recv_list[i]) {
            delete _recv_list[i];
        }
    }
    delete _pri_queue;
}

void SendMsgToPriQueue(NetMsg* msg) {

}

void SendMsgToSendWnd(NetMsg* msg) {

}

void SendMsgToNet(NetMsg* msg) {

}

void RecvMsgUpper(NetMsg* msg) {

}

void RecvMsgToOrderList(NetMsg* msg) {

}