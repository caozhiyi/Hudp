#ifndef HEADER_INTERFACE_FILTERPROCESS
#define HEADER_INTERFACE_FILTERPROCESS

#include <memory> // for shared_ptr
#include <string> // for string
#include <functional>

namespace hudp {

    class CMsg;
    class CFilter;
    class CFilterProcess {
    public:
        CFilterProcess() {}
        virtual ~CFilterProcess() {}
        // add a filter
        virtual void AddFilter(std::shared_ptr<CFilter> filter) = 0;
        // push message to send process
        virtual bool PushSendMsg(const HudpHandle& handle, uint16_t flag, std::string& body) = 0;
        // push message to recv process
        virtual bool PushRecvMsg(const HudpHandle& handle, uint16_t flag, std::string& body) = 0;
    };
}
#endif