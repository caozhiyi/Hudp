#ifndef HEADER_INTERFACE_SOCKET
#define HEADER_INTERFACE_SOCKET

#include "CommonType.h"

namespace hudp {
    class CMsg;
    class CSocket
    {
    public:
        CSocket(const Handle& handle) {}
        virtual ~CSocket() {}

        virtual Handle GetHandle() = 0;

        virtual void PushMsgToPriQueue(CMsg* msg) = 0;
        virtual CMsg* GetMsgFromPriQueue() = 0;

        virtual void PushMsgToSendWnd(CMsg* msg) = 0;

        // called back by order list when msg recv to upper.
        virtual void ToRecv(CMsg* msg) = 0;
        // called back by send window t when send a bag to net.
        virtual void ToSend(CMsg* msg) = 0;
        // called back by send window t when recv a ack.
        virtual void AckDone(CMsg* msg) = 0;
        // called back by timer t when timer out.
        virtual void TimerOut(CMsg* msg) = 0;
    };
}
#endif