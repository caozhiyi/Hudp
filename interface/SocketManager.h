#ifndef HEADER_INTERFACE_SOCKETMANAGER
#define HEADER_INTERFACE_SOCKETMANAGER

#include <memory> // for share_ptr
#include "CommonType.h"

namespace hudp {
    class CSocket;
    // socket manager interface.
    class CSocketManager
    {
    public:
        CSocketManager() {}
        virtual ~CSocketManager() {}

        virtual bool IsSocketExist(const Handle& handle) = 0;
        // return a socket, create one if not exist.
        virtual std::shared_ptr<CSocket> GetSocket(const Handle& handle) = 0;

        virtual void DeleteSocket(const Handle& handle) = 0;
    };
}
#endif