#ifndef HEADER_HUDP_RECVTHREAD
#define HEADER_HUDP_RECVTHREAD

#include "RunnableAloneTaskList.h"

namespace hudp {

    class CRecvThread : public base::CRunnable {
    public:
        CRecvThread();
        ~CRecvThread();

        virtual void Start(uint64_t socket);
        virtual void Run();
    private:
        uint64_t    _recv_socket;
    };

}

#endif