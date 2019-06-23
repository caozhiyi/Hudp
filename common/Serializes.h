#ifndef HEADER_COMMON_SERIALIZES
#define HEADER_COMMON_SERIALIZES

#include "CommonType.h"

namespace hudp {
    class CHudpBitStream;
    class NetMsg;
    class Head;

    class CSerializes {
    public:
        static bool Serializes(NetMsg& msg, CHudpBitStream& bit_stream);
        static bool Serializes(const Head& head, CHudpBitStream& bit_stream);
        static bool Serializes(Head& head, const char* body, uint16_t len, CHudpBitStream& bit_stream);

        static bool Deseriali(CHudpBitStream& bit_stream, NetMsg& msg);
        static bool Deseriali(CHudpBitStream& bit_stream, Head& head);
        static bool Deseriali(CHudpBitStream& bit_stream, Head& head, char* body, uint16_t& len);
    };
}

#endif