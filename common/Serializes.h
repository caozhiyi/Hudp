#ifndef HEADER_COMMON_SERIALIZES
#define HEADER_COMMON_SERIALIZES

#include "CommonType.h"
#include "ISerializes.h"

namespace hudp {

    class CBitStreamWriter;
    class CBitStreamReader;
    class Head;

    class CSerializesNormal : public CSerializes {
    public:
        bool Serializes(CMsg& msg, CBitStreamWriter& bit_stream);
        bool Deseriali(CBitStreamReader& bit_stream, CMsg& msg);
        // get estimate msg size
        uint32_t EstimateSize(CMsg& msg);
    private:
        bool SerializesHead(Head& head, CBitStreamWriter& bit_stream);
        bool Serializes(Head& head, const char* body, uint16_t len, CBitStreamWriter& bit_stream);
    
        bool DeserialiHead(CBitStreamReader& bit_stream, Head& head);
        bool Deseriali(CBitStreamReader& bit_stream, Head& head, std::string& body);
    };
}

#endif
