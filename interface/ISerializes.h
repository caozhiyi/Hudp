#ifndef HEADER_INTERFACE_SERIALIZES
#define HEADER_INTERFACE_SERIALIZES

#include "CommonType.h"

namespace hudp {

    class CBitStreamWriter;
    class CBitStreamReader;
    class CMsg;

    class CSerializes {
    public:
        bool Serializes(CMsg& msg, CBitStreamWriter& bit_stream);

        bool Deseriali(CBitStreamReader& bit_stream, CMsg& msg);
    };
}

#endif
