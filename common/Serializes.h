#ifndef HEADER_COMMON_SERIALIZES
#define HEADER_COMMON_SERIALIZES

#include "Common.h"

namespace hudp {
    class CHudpBitStream;

    bool Serializes(NetMsg& msg, CHudpBitStream& stream);
    bool Serializes(const Head& head, CHudpBitStream& stream);
    bool Serializes(Head& head, const char* body, uint16_t len, CHudpBitStream& stream);

    bool Deseriali(CHudpBitStream& stream, NetMsg& msg);
    bool Deseriali(CHudpBitStream& stream, Head& head);
    bool Deseriali(CHudpBitStream& stream, Head& head, char* body, uint16_t& len);
}

#endif