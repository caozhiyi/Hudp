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
    };
}
#endif