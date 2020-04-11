#ifndef HEADER_HUDP_FILTERPROCESSWITHTHREAD
#define HEADER_HUDP_FILTERPROCESSWITHTHREAD

#include <string> // for string
#include <memory> // for share_ptr
#include <vector> // for vector

#include "CommonType.h"
#include "IFilterProcess.h"
#include "RunnableAloneTaskList.h"

namespace hudp {

    struct FilterProcessParam {
        enum FilterType {
            FILTER_RECV = 1,
            FILTER_SEND = 2
        };
        FilterType  _filter_type;
        HudpHandle  _handle;
        uint16_t    _flag;
        std::string _body;
        FilterProcessParam(FilterType filter_type, const HudpHandle& handle, uint16_t flag, const std::string&& body) :
            _filter_type(filter_type), _handle(handle), _flag(flag), _body(std::move(body)) { }
    };
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
