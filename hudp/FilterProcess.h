#ifndef HEADER_HUDP_FILTERPROCESS
#define HEADER_HUDP_FILTERPROCESS

#include <string>
#include "CommonFlag.h"
#include "Single.h"

namespace hudp {

    class NetMsg;
    class CFilterInterface;
    class CFilterProcess : base::CSingle<CFilterProcess> {
    public:
        CFilterProcess() {}
        virtual ~CFilterProcess() {}
        
        // process chain of responsibility
        void SendProcess(NetMsg* msg);
        void RecvProcess(NetMsg* msg);

        // add a filter to responsibility
        bool Add(CFilterInterface* filter);
        // remove a filter to responsibility
        bool Remove(CFilterInterface* filter);

    private:
        // get start index from vector 
        size_t GetIndex(process_phase cur_handle_phase);

    private:
        std::vector<CFilterInterface*> _filer_list;
        process_phase _cur_handle_phase;
    };
}

#endif