#ifndef HEADER_HUDP_FILTERPROCESSWITHTHREAD
#define HEADER_HUDP_FILTERPROCESSWITHTHREAD

#include <string> // for string
#include <memory> // for share_ptr
#include <vector> // for vector

#include "CommonType.h"
#include "IFilterProcess.h"
#include "FilterProcessParam.h"
#include "RunnableAloneTaskList.h"

namespace hudp {

    class CFilterProcessWithThread: public CFilterProcess,
                                    public base::CRunnableAloneTaskList<FilterProcessParam> {
    public:
        CFilterProcessWithThread();
        ~CFilterProcessWithThread();
        
        void AddFilter(std::shared_ptr<CFilter> filter);
        // push message to send process
        bool PushSendMsg(const HudpHandle& handle, uint16_t flag, std::string& body);
        // push message to recv process
        bool PushRecvMsg(const HudpHandle& handle, uint16_t flag, std::string& body);

        // thread about
        void Run();
    private:
        std::shared_ptr<CFilter> _filter_head;
        std::shared_ptr<CFilter> _filter_end;
    };
}

#endif
