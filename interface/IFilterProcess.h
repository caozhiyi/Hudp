#ifndef HEADER_INTERFACE_FILTERPROCESS
#define HEADER_INTERFACE_FILTERPROCESS

#include <memory> // for shared_ptr
#include <string> // for string
#include <functional>

namespace hudp {

    class CMsg;
    class CFilter;
    // manage all filters.
    // filters are organized as a two-way linked list,
    // called from list header to list tail when sending message,
    // called from list tail to list header when recving message,
    // so pay attention to the location where the filter is added.
    class CFilterProcess {
    public:
        CFilterProcess() {}
        virtual ~CFilterProcess() {}
        // add a filter
        virtual void AddFilter(std::shared_ptr<CFilter> filter) = 0;
        // push message to send process
        virtual bool PushSendMsg(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id = 0) = 0;
        // push message to recv process
        virtual bool PushRecvMsg(const HudpHandle& handle, uint16_t flag, std::string& body) = 0;
    };
}
#endif