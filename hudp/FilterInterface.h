#ifndef HEADER_HUDP_FILTERINTERFACE
#define HEADER_HUDP_FILTERINTERFACE

#include "Common.h"

namespace hudp {

    static     

    class CFilterInterface {
        CFilterInterface() {}
        virtual ~CFilterInterface() {}
        
        virtual bool OnSend(NetMsg* msg) {}

        virtual bool OnRecv(NetMsg* msg) {}

    private:
        process_phase _handle_phase;
    
    };
}

#endif