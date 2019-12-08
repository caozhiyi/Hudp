#ifndef HEADER_HUDP_RECVTHREAD
#define HEADER_HUDP_RECVTHREAD

#include "RunnableAloneTaskList.h"

namespace hudp {

    class CMsgFactory;
    class CNetIO;
    class CRecvThread : public base::CRunnable {
    public:
        CRecvThread();
        ~CRecvThread();

        virtual void Start(uint64_t sock, std::shared_ptr<CMsgFactory>& msg_factory, std::shared_ptr<CNetIO>& net_io);
        virtual void Run();
    private:
        uint64_t                        _recv_socket;
        std::shared_ptr<CMsgFactory>    _msg_factory;
        std::shared_ptr<CNetIO>         _net_io;
    };

}

#endif