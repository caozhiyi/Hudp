#ifndef HEADER_HUDP_FILTERPROCESS
#define HEADER_HUDP_FILTERPROCESS

#include <string> // for string
#include <memory> // for share_ptr
#include <vector> // for vector

#include "IFilterProcess.h"

namespace hudp {

    class CFilter;
    class CMsg;
    class CFilterProcessNoThread: public CFilterProcess {
    public:
        CFilterProcessNoThread() {}
        ~CFilterProcessNoThread() {}
        
        bool AddFilter(const std::shared_ptr<CFilter>& filter);
        // remove a filter
        bool RemoveFilter(const std::shared_ptr<CFilter>& filter);
        // push message to send process
        void PushSendMsg(std::shared_ptr<CMsg> msg);
        // set send message call back
        void SetSendFunc(const std::function<void(std::shared_ptr<CMsg>)>& func);
        // push message to recv process
        void PushRecvMsg(std::shared_ptr<CMsg> msg);
        // set recv message call back
        void SetRecvFunc(const std::function<void(std::shared_ptr<CMsg>)>& func);

    private:
        void SendProcess(std::shared_ptr<CMsg> msg);
        void RecvProcess(std::shared_ptr<CMsg> msg);

    private:
        std::vector<std::shared_ptr<CFilter>> _filer_vec;
        std::function<void(std::shared_ptr<CMsg>)>            _recv_call_back;
        std::function<void(std::shared_ptr<CMsg>)>            _send_call_back;
    };
}

#endif
