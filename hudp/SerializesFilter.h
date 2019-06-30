#ifndef HEADER_HUDP_SERIALIZEFILER
#define HEADER_HUDP_SERIALIZEFILER

#include "FilterInterface.h"

namespace hudp {

    class CBitStream;
    // protocol resolution
    class CSerializesFilter : public CProtocolFilterInterface {
    public:
        CSerializesFilter() {}
        virtual ~CSerializesFilter() {}

        virtual bool OnSend(NetMsg* msg);
        virtual bool OnRecv(NetMsg* msg);
    };

}

#endif