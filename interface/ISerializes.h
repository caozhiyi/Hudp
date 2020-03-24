#ifndef HEADER_INTERFACE_SERIALIZES
#define HEADER_INTERFACE_SERIALIZES

#include "CommonType.h"

namespace hudp {

    class CBitStreamWriter;
    class CBitStreamReader;
    class CMsg;

    class CSerializes {
    public:
        virtual bool Serializes(CMsg& msg, CBitStreamWriter& bit_stream) = 0;

        virtual bool Deseriali(CBitStreamReader& bit_stream, CMsg& msg) = 0;

        // get estimate msg size
        virtual uint32_t EstimateSize(CMsg& msg) = 0;
    };
}

#endif
