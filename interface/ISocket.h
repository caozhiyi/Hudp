#ifndef HEADER_INTERFACE_SOCKET
#define HEADER_INTERFACE_SOCKET

#include <memory> // for shared_ptr
#include "CommonType.h"

namespace hudp {
    class CMsg;
    class CSocket {
    public:
        CSocket() {}
        virtual ~CSocket() {}

        virtual HudpHandle GetHandle() = 0;

        virtual void SendMessage(std::shared_ptr<CMsg> msg) = 0;

        virtual void RecvMessage(std::shared_ptr<CMsg> msg) = 0;

        // called back by order list when msg recv to upper.
        virtual void ToRecv(std::shared_ptr<CMsg> msg) = 0;
        // called back by send window when send a bag to net.
        virtual void ToSend(std::shared_ptr<CMsg> msg) = 0;
        // called back by send window when recv a ack.
        virtual void AckDone(std::shared_ptr<CMsg> msg) = 0;
        // called back by timer when timer out.
        virtual void TimerOut(std::shared_ptr<CMsg> msg) = 0;
        // send fin message to remote to close connection
        virtual void SendFinMessage() = 0;
        // can send msg?
        virtual bool CanSendMessage() = 0;
    };
}
#endif