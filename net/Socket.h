#ifndef HEADER_NET_SOCKET
#define HEADER_NET_SOCKET

#include "CommonType.h"

namespace hudp {

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

        NetMsg* GetMsgFromPriQueue();

        void SendMsgToPriQueue(NetMsg* msg);
        void SendMsgToSendWnd(NetMsg* msg);
        void SendMsgToNet(NetMsg* msg);

        void RecvMsgUpper(NetMsg* msg);
        void RecvMsgToOrderList(NetMsg* msg);

    private:
        // Instantiating corresponding classes
        void CreateSendWnd(WndIndex index);
        void CreateRecvList(WndIndex index);

    private:
        CIncrementalId* _inc_id[__wnd_size];
        CSendWnd*       _send_wnd[__wnd_size];
        CRecvList*      _recv_list[__wnd_size];
        CPriorityQueue* _pri_queue;
    };
}

#endif