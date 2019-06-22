#ifndef HEADER_COMMON_SERIALIZES
#define HEADER_COMMON_SERIALIZES

namespace hudp {
    class CHudpBitStream;
    class NetMsg;
    class Head;

    class CSerializes {
    public:
        static bool Serializes(NetMsg& msg, CHudpBitStream& stream);
        static bool Serializes(const Head& head, CHudpBitStream& stream);
        static bool Serializes(Head& head, const char* body, uint16_t len, CHudpBitStream& stream);

        static bool Deseriali(CHudpBitStream& stream, NetMsg& msg);
        static bool Deseriali(CHudpBitStream& stream, Head& head);
        static bool Deseriali(CHudpBitStream& stream, Head& head, char* body, uint16_t& len);
    };
}

#endif