#ifndef HEADER_OSNET_INTERFACE
#define HEADER_OSNET_INTERFACE

#include "CommonType.h"

namespace hudp {

    static const uint8_t __ip_length = 32;
    static const uint8_t __wnd_size  = 3;

    enum WndIndex {
        WI_ORDERLY          = 0,
        WI_RELIABLE         = 1,
        WI_RELIABLE_ORDERLY = 2
    };

    class CSendWnd;
    class CRecvList;
    class CPriorityQueue;
    class CIncrementalId;
    class NetMsg;

    class CSocket {
    public:
        CSocket();
        ~CSocket();

        void SendMsg(NetMsg* msg);
        void RecvMsg(NetMsg* msg);

    private:
        char            _ip[__ip_length];
        uint16_t        _short;
        uint64_t    	_socket;

        CIncrementalId* _inc_id[__wnd_size];
        CSendWnd*       _send_wnd[__wnd_size];
        CRecvList*      _recv_list[__wnd_size];
        CPriorityQueue* _pri_queue;
    };
}

#endif