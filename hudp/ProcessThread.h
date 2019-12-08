#ifndef HEADER_HUDP_SENDTHREAD
#define HEADER_HUDP_SENDTHREAD

#include "RunnableAloneTaskList.h"

namespace hudp {

    class CFilterProcess;
    class CMsg;
    class CProcessThread : public base::CRunnableAloneTaskList<CMsg*> {
    public:
        CProcessThread();
        ~CProcessThread();

        virtual void Start(std::shared_ptr<CFilterProcess>& filter_process);
        virtual void Run();

    private:
        std::shared_ptr<CFilterProcess> _filter_process;
    };
}

#endif