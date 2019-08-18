#ifndef HEADER_NET_CLOSEMANAGER
#define HEADER_NET_CLOSEMANAGER

#include "CommonFlag.h"
#include "Single.h"

namespace hudp {

    class CSocket;
    class NetMsg;
    class CCloseManager : public base::CSingle<CCloseManager> {
    public:
        CCloseManager();
        ~CCloseManager();
    
        // start close process
        void StartClose(const HudpHandle& handle);

        // recv a close msg from remote.
        void RecvClose(const HudpHandle& handle);

        // recv a close msg ack from remote.
        bool CloseAck(const HudpHandle& handle);
    };
}

#endif