#ifndef HEADER_HUDP_RELIALEORDERLYFILER
#define HEADER_HUDP_RELIALEORDERLYFILER

#include "FilterInterface.h"

namespace hudp {

    class CBitStream;
    // protocol resolution
    class CRelialeOrderlyFilter : public CHeadFilterInterface {
    public:
        CRelialeOrderlyFilter() {}
        virtual ~CRelialeOrderlyFilter() {}

        virtual bool OnSend(NetMsg* msg);
        virtual bool OnRecv(NetMsg* msg);
    };

}

#endif