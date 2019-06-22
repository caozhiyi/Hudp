#ifndef HEADER_HUDP_SERIALIZEFILER
#define HEADER_HUDP_SERIALIZEFILER

#include "FilterInterface.h"

namespace hudp {

    class CHudpBitStream;
    // protocol resolution
    class CSerializesFilter : public CProtocolFilterInterface {
    public:
        CSerializesFilter() {}
        virtual ~CSerializesFilter() {}

        virtual bool OnSend(NetMsg* msg, CHudpBitStream* bit_stream, const std::string& _ip_port);
        virtual bool OnRecv(CHudpBitStream* bit_stream, NetMsg* msg, const std::string& _ip_port);
    };

}

#endif