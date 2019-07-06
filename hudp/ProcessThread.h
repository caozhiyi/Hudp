#ifndef HEADER_HUDP_PROCESSTHREAD
#define HEADER_HUDP_PROCESSTHREAD

#include "RunnableAloneTaskList.h"

namespace hudp {

    class CRecvProcessThread : public base::CRunnableAloneTaskList<void*> {
    public:
        CRecvProcessThread();
        ~CRecvProcessThread();

        virtual void Run(); 
    };

    class CSendProcessThread : public base::CRunnableAloneTaskList<void*> {
    public:
        CSendProcessThread();
        ~CSendProcessThread();

        virtual void Run();
    };

}

#endif