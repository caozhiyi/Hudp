#ifndef HEADER_INTERFACE_FILTERPROCESS
#define HEADER_INTERFACE_FILTERPROCESS

#include <string> // for string
#include <functional>

namespace hudp {
    class CFilter;
    class CFilterProcess
    {
    public:
        CFilterProcess() {}
        virtual ~CFilterProcess() {}
        // add a filter
        virtual bool AddFilter(const std::shared_ptr<CFilter>& filter) = 0;
        // remove a filter
        virtual bool RemoveFilter(const std::shared_ptr<CFilter>& filter) = 0;
        // push message to send process
        virtual void PushSendMsg(CMsg* msg) = 0;
        // set send message call back
        virtual void SetSendFunc(const std::function<void(std::string&)>& func) = 0;
        // push message to recv process
        virtual void PushRecvMsg(CMsg* msg) = 0;
        // set recv message call back
        virtual void SetRecvFunc(const std::function<void(std::string&)>& func) = 0;
    };
}
#endif