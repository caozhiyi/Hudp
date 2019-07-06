#ifndef HEADER_HUDP_SENDTHREAD
#define HEADER_HUDP_SENDTHREAD

#include "RunnableAloneTaskList.h"

namespace hudp {

    class CSendThread : public base::CRunnableAloneTaskList<void*> {
    public:
        CSendThread();
        ~CSendThread();

        virtual void Start(uint64_t socket);
        virtual void Run();
    private:
        uint64_t _send_socket;
    };
}

#endif