#ifndef HEADER_HUDP_FILTERPROCESS
#define HEADER_HUDP_FILTERPROCESS

#include <string>
#include "CommonFlag.h"
#include "Single.h"

namespace hudp {

    class NetMsg;
    class CFilterInterface;
    class CFilterProcess : public base::CSingle<CFilterProcess> {
    public:
        CFilterProcess() {}
        virtual ~CFilterProcess() {}
        
        // init filter
        void Init();

        // process chain of responsibility
        void SendProcess(NetMsg* msg);
        void RecvProcess(NetMsg* msg);

        // add a filter to responsibility
        bool Add(CFilterInterface* filter);
        // remove a filter to responsibility
        bool Remove(CFilterInterface* filter);

    private:
        std::vector<CFilterInterface*> _filer_list;
    };
}

#endif