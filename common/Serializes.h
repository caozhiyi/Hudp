#ifndef HEADER_COMMON_SERIALIZES
#define HEADER_COMMON_SERIALIZES

#include "CommonType.h"

namespace hudp {

    class CBitStreamWriter;
    class CBitStreamReader;
    class CMsgImpl;
    class Head;

    class CSerializes {
    public:
        static bool Serializes(CMsgImpl& msg, CBitStreamWriter& bit_stream);
        static bool Serializes(const Head& head, CBitStreamWriter& bit_stream);
        static bool Serializes(Head& head, const char* body, uint16_t len, CBitStreamWriter& bit_stream);

        static bool Deseriali(CBitStreamReader& bit_stream, CMsgImpl& msg);
        static bool Deseriali(CBitStreamReader& bit_stream, Head& head);
        static bool Deseriali(CBitStreamReader& bit_stream, Head& head, std::string& body);
    };
}

#endif
