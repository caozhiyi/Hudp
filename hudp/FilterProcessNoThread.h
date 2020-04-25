#ifndef HEADER_HUDP_FILTERPROCESSNOTHREAD
#define HEADER_HUDP_FILTERPROCESSNOTHREAD

#include <string> // for string
#include <memory> // for share_ptr
#include <vector> // for vector

#include "CommonType.h"
#include "IFilterProcess.h"

namespace hudp {

    class CMsg;
    class CFilter;
    class CFilterProcessNoThread: public CFilterProcess {
    public:
        CFilterProcessNoThread();
        ~CFilterProcessNoThread();
        
        void AddFilter(std::shared_ptr<CFilter> filter);
        // push message to send process
        bool PushSendMsg(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id);
        // push message to recv process
        bool PushRecvMsg(const HudpHandle& handle, uint16_t flag, std::string& body);

    private:
        std::shared_ptr<CFilter> _filter_head;
        std::shared_ptr<CFilter> _filter_end;
    };
}

#endif
