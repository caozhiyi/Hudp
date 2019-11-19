#ifndef HEADER_NET_SOCKET
#define HEADER_NET_SOCKET

#include <memory>       // for enable shared from this
#include "CommonType.h"
#include "Socket.h"

namespace hudp {

    static const uint8_t __wnd_size  = 3;

    enum WndIndex {
        WI_ORDERLY          = 0,
        WI_RELIABLE         = 1,
        WI_RELIABLE_ORDERLY = 2
    };

    class CSendWnd;
    class COrderList;
    class CPriorityQueue;
    class CIncrementalId;
    class CMsg;
    class CPendAck;
    class CRto;

    class CSocketImpl : public CSocket, public std::enable_shared_from_this<CSocketImpl> {
    public:
        CSocketImpl(const Handle& handle);
        ~CSocketImpl();

        Handle GetHandle();

        void SendMessage(CMsg* msg);

        void RecvMessage(CMsg* msg);

        // called back by order list when msg recv to upper.
        void ToRecv(CMsg* msg);
        // called back by send window t when send a bag to net.
        void ToSend(CMsg* msg, CSendWnd* send_wnd);
        // called back by send window t when recv a ack.
        void AckDone(CMsg* msg);
        // called back by timer t when timer out.
        void TimerOut(CMsg* msg);

        // get msg from pri queue. 
        // not blocked. return null if empty.
        NetMsg* GetMsgFromPriQueue();

        // send msg to module of socket.
        void SendMsgToPriQueue(NetMsg* msg);
        void SendMsgToSendWnd(NetMsg* msg);
        void SendMsgToNet(NetMsg* msg);

        // accept ack to send wnd
        void RecvAck(NetMsg* msg);
        // recv msg to module of socket.
        void RecvMsgUpper(NetMsg* msg);
        void RecvMsgToOrderList(NetMsg* msg);

        // add a ack msg to remote 
        void AddAck(NetMsg* msg);
        // attach ack info to msg
        bool AttachPendAck(NetMsg* msg);

        // timer call back
        void OnTimer();

        // add a msg to timer. return cur time stamp.
        uint64_t AddToTimer(CSenderRelialeOrderlyNetMsg* msg);


        // get ip port handle
        HudpHandle GetHandle();

    private:
        void AddAckToMsg(CMsg* msg);
        void GetAckToSendWnd(CMsg* msg);
    private:
        void AddToSendWnd(WndIndex index, CMsg* msg);
        void AddToRecvList(WndIndex index, CMsg* msg);
        void AddToPendAck(WndIndex index, CMsg* msg);
        void AddToPriorityQueue(WndIndex index, CMsg* msg);

    private:
        // reliable correlation
        CSendWnd*            _send_wnd[__wnd_size];
        CRecvList*           _recv_list[__wnd_size];
        CPendAck*            _pend_ack[__wnd_size];
        // msg priority queue
        CPriorityQueue*      _pri_queue[__wnd_size];
        // about pend ack timer
        std::atomic<bool>    _is_in_timer;
        // rto
        CRto*                _rto;
        // socket handle
        Handle               _handle;
    };
}

#endif