#ifndef HEADER_FILTER_UPPER_THREAD_FILTER
#define HEADER_FILTER_UPPER_THREAD_FILTER

#include "IFilter.h"
#include "FilterProcessParam.h"
#include "RunnableAloneTaskList.h"

namespace hudp {
    // the first filter
    // send msg to upper
    class CUpperThreadFilter : public CFilter,
                               public base::CRunnableAloneTaskList<FilterProcessParam> {
    public:
        CUpperThreadFilter();
        virtual ~CUpperThreadFilter();
        // when send msg filter process
        bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id);
        // when recv msg filter process
        bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
        // thread about
        void Run();
    };
}
#endif
