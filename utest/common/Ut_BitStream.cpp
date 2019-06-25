#include <iostream>
#include "BitStream.h"

using namespace hudp;

void UtestBitStream() {
    CHudpBitStream bit_stream;

    std::cout << "total length : " << bit_stream.GetTotalLength() << std::endl;
    std::cout << "cur   length : " << bit_stream.GetCurrentLength() << std::endl;
    
    
}

int main() {
    UtestBitStream();

    int a;
    a++;
}

        //// get length interface
        //uint16_t GetTotalLength() const;
        //uint16_t GetCurrentLength() const;

        //const char* GetDataPoint() const;

        //void Clear();
        //
        //// only can write type that defined in common type
        //template<typename T>
        //bool Write(T& value);
        //bool Write(const char* value, uint16_t len);
        //bool Write(const std::string& value);
        //bool Write(const CHudpBitStream& value);

        //template<typename T>
        //bool Read(T& value);
        //bool Read(char* value, uint16_t len);
        //bool Read(std::string& value, uint16_t len);
        //bool Read(CHudpBitStream& value, uint16_t len);
