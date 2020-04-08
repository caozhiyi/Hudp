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

    class CBbr;
    class CRto;
    class CMsg;
    class CPacing;
    class CPendAck;
    class CSendWnd;
    class CFlowQueue;
    class COrderList;
    class CPriorityQueue;
    class CIncrementalId;

    class CSocketImpl : public CSocket, public std::enable_shared_from_this<CSocketImpl> {
    public:
        CSocketImpl(const HudpHandle& handle);
        ~CSocketImpl();
        // get scocket handle. as "ip:port"
        HudpHandle GetHandle();
        // send message to the socket
        void SendMessage(std::shared_ptr<CMsg> msg);
        // recv msg from net
        void RecvMessage(std::shared_ptr<CMsg> msg);

        // called back by order list when msg recv to upper.
        void ToRecv(std::shared_ptr<CMsg> msg);
        // called back by send window t when send a bag to net.
        void ToSend(std::shared_ptr<CMsg> msg);
        // called back by send window t when recv a ack.
        void AckDone(std::shared_ptr<CMsg> msg);
        // called back by timer t when timer out.
        void TimerOut(std::shared_ptr<CMsg> msg);
        // send fin msg to close connection
        void SendFinMessage();
        // check socket status can send message now?
        bool CanSendMessage();
    private:
        // add a ack msg timer to remote 
        void AddPendAck(std::shared_ptr<CMsg> msg);
        // send a ack quickly
        void AddQuicklyAck(std::shared_ptr<CMsg> msg);
        // return ture if add ack to msg, otherwise return false
        bool AddAckToMsg(std::shared_ptr<CMsg> msg);
        // get ack info from msg and add to send wnd.
        void GetAckToSendWnd(std::shared_ptr<CMsg> msg);
        // add msg to send wnd
        void AddToSendWnd(WndIndex index, std::shared_ptr<CMsg> msg);
        // add msg to recv order list
        void AddToRecvList(WndIndex index, std::shared_ptr<CMsg> msg);
        // add msg ack to pend queue
        void AddToPendAck(WndIndex index, std::shared_ptr<CMsg> msg);
        // calculation rtt time
        uint64_t GetRtt(std::shared_ptr<CMsg> msg);
        uint64_t GetRtt(uint64_t time);
        // socket status change
        void StatusChange(socket_status sk_status);
        // send fin ack
        void SendFinAckMessage();
        // send reset msg
        void SendResetMsg();
        // add 2msl timer
        void Wait2MslClose();
        // about close flag
        void CheckClose(uint32_t header_flag);
        // send msg by FQ and pacing
        void SendPacingMsg(std::shared_ptr<CMsg> msg, bool add_fq = true);
        // controller algorithm process
        void ControllerProcess(WndIndex index, uint32_t rtt, uint32_t acked, uint32_t delivered);
        // pacing send msg call back
        void PacingCallBack(std::shared_ptr<CMsg> msg);

    private:
        // reliable correlation
        CSendWnd*            _send_wnd[__wnd_size];
        COrderList*          _recv_list[__wnd_size];
        CPendAck*            _pend_ack[__wnd_size];
 
        // about pend ack timer
        std::atomic<bool>      _is_in_timer;
        // rto calc
        std::shared_ptr <CRto> _rto;
        // socket handle
        HudpHandle             _handle;
        // socket status
        socket_status          _sk_status;

        // count lost msg size while a ack turn
        std::atomic<uint32_t>       _lost_msg;
        // count msg size in flight
        std::atomic<uint32_t>       _in_flight;
        // flow conntroller about
        std::shared_ptr<CBbr>       _bbr_controller;
        std::shared_ptr<CPacing>    _pacing;
        std::shared_ptr<CFlowQueue> _flow_queue;
    };
}

#endif