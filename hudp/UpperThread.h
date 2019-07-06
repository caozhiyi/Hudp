#ifndef HEADER_HUDP_UPPERTHREAD
#define HEADER_HUDP_UPPERTHREAD

#include <string>
#include <functional>
#include "RunnableAloneTaskList.h"
#include "CommonFlag.h"

namespace hudp {

    class CUpperThread : public base::CRunnableAloneTaskList<void*> {
    public:
        CUpperThread();
        ~CUpperThread();

        virtual void Start(const recv_back& func);
        virtual void Run();
    private:
        recv_back _call_back;
    };
}

#endif