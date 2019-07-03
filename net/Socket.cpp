#include <cstring>		//for memset
#include "Socket.h"

using namespace hudp;

CSocket::CSocket() : _short(0), _socket(0), _pri_queue(nullptr) {
    memset(_ip, 0, sizeof(_ip));
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
}

void CSocket::SendMsg(NetMsg* msg) {

}

void CSocket::RecvMsg(NetMsg* msg) {

}