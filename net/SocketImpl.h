#ifndef HEADER_NET_SOCKETIMPL
#define HEADER_NET_SOCKETIMPL

#include <atomic>
#include <memory>       // for enable shared from this

#include "ISocket.h"
#include "HudpFlag.h"
#include "CommonType.h"
#include "CommonFlag.h"

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
        CSocketImpl(const HudpHandle& handle);
        ~CSocketImpl();

        HudpHandle GetHandle();

        void SendMessage(CMsg* msg);

        void RecvMessage(CMsg* msg);

        // called back by order list when msg recv to upper.
        void ToRecv(CMsg* msg);
        // called back by send window t when send a bag to net.
        void ToSend(CMsg* msg);
        // called back by send window t when recv a ack.
        void AckDone(CMsg* msg);
        // called back by timer t when timer out.
        void TimerOut(CMsg* msg);
        // add a ack msg timer to remote 
        void AddPendAck(CMsg* msg);
        // send a ack quickey
        void AddQuicklyAck(CMsg* msg);
    
        // send fin msg to close connection
        void SendFinMessage();
        bool CanSendMessage();
    private:
        // return ture if add ack to msg, otherwise return false
        bool AddAckToMsg(CMsg* msg);
        void GetAckToSendWnd(CMsg* msg);
        void AddToSendWnd(WndIndex index, CMsg* msg);
        void AddToRecvList(WndIndex index, CMsg* msg);
        void AddToPendAck(WndIndex index, CMsg* msg);
        // calculation rtt time
        uint64_t GetRtt(CMsg* msg);
        uint64_t GetRtt(uint64_t time);
        // socket status change
        void StatusChange(socket_status sk_status);
        // send fin ack
        void SendFinAckMessage();
        // add 2msl timer
        void Wait2MslClose();
        // about close flag
        void CheckClose(uint32_t header_flag);

    private:
        // reliable correlation
        CSendWnd*            _send_wnd[__wnd_size];
        COrderList*          _recv_list[__wnd_size];
        CPendAck*            _pend_ack[__wnd_size];
 
        // about pend ack timer
        std::atomic<bool>    _is_in_timer;
        // rto
        CRto*                _rto;
        // socket handle
        HudpHandle           _handle;
        // socket status
        socket_status        _sk_status;
    };
}

#endif