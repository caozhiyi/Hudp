#ifndef HEADER_HUDP_FILTERPROCESS
#define HEADER_HUDP_FILTERPROCESS

#include <string>

namespace hudp {

    class NetMsg;
    class CFilterProcess {
    public:
        CFilterProcess() {}
        virtual ~CFilterProcess() {}
        
        // process chain of responsibility
        void SendProcess(NetMsg* msg);
        void RecvProcess(NetMsg* msg);
    };
}

#endif