#include "BitStream.h"
#include "../Test.h"
using namespace hudp;

void UtestBitStream() {
    CBitStreamWriter bit_stream;

    std::cout << "init status: " << std::endl;
    std::cout << "total length : " << bit_stream.GetTotalLength() << std::endl;
    std::cout << "cur   length : " << bit_stream.GetCurrentLength() << std::endl;

    uint16_t uint16 = 12345;
    uint32_t uint32 = 12345678;
    uint64_t uint64 = 12345678910;
    char     buf[]  = "AqweqweqweasdqweA";
    uint16_t buf_len= strlen(buf);
    std::string str = "BasdadasdasdadasB";
    
    Expect_True(bit_stream.Write(uint16));
    Expect_True(bit_stream.Write(uint32));
    Expect_True(bit_stream.Write(uint64));
    Expect_True(bit_stream.Write(buf, buf_len));
    Expect_True(bit_stream.Write(str));
    
    uint16_t bit_len = bit_stream.GetCurrentLength();
    Expect_True(bit_stream.Write(bit_stream));


    std::cout << "write: " << std::endl;
    std::cout << "cur   uint16 : " << uint16 << std::endl;
    std::cout << "cur   uint32 : " << uint32 << std::endl;
    std::cout << "cur   uint64 : " << uint64 << std::endl;
    std::cout << "cur   char*  : " << buf << std::endl;
    std::cout << "cur   string : " << str << std::endl;

    std::cout << "cur   length : " << bit_stream.GetCurrentLength() << std::endl;
    std::cout << "cur   point  : " << bit_stream.GetDataPoint() << std::endl;

    uint16 = 0;
    uint32 = 0;
    uint64 = 0;
    memset(buf, 0, buf_len);
    str.clear();

    CBitStreamReader bit_stream_2;
    bit_stream_2.Init(bit_stream.GetDataPoint(), bit_stream.GetCurrentLength());

    //bit_stream_2.Clear();

    Expect_True(bit_stream_2.Read(uint16));
    Expect_True(bit_stream_2.Read(uint32));
    Expect_True(bit_stream_2.Read(uint64));
    Expect_True(bit_stream_2.Read(buf, buf_len));
    Expect_True(bit_stream_2.Read(str, buf_len));
    //Expect_True(bit_stream_2.Read(str, buf_len));

    std::cout << "read1: " << std::endl;
    std::cout << "cur   uint16 : " << uint16 << std::endl;
    std::cout << "cur   uint32 : " << uint32 << std::endl;
    std::cout << "cur   uint64 : " << uint64 << std::endl;
    std::cout << "cur   char*  : " << buf << std::endl;
    std::cout << "cur   string : " << str << std::endl;

    std::cout << "cur   length : " << bit_stream_2.GetCurrentLength() << std::endl;
    std::cout << "cur   point  : " << bit_stream_2.GetDataPoint() << std::endl;

    CBitStreamReader bit_stream_3;
    Expect_True(bit_stream_2.Read(bit_stream_3, bit_len));

    uint16 = 0;
    uint32 = 0;
    uint64 = 0;
    memset(buf, 0, buf_len);
    str.clear();

    Expect_True(bit_stream_3.Read(uint16));
    Expect_True(bit_stream_3.Read(uint32));
    Expect_True(bit_stream_3.Read(uint64));
    Expect_True(bit_stream_3.Read(buf, buf_len));
    Expect_True(bit_stream_3.Read(str, buf_len));

    std::cout << "read2: " << std::endl;
    std::cout << "cur   uint16 : " << uint16 << std::endl;
    std::cout << "cur   uint32 : " << uint32 << std::endl;
    std::cout << "cur   uint64 : " << uint64 << std::endl;
    std::cout << "cur   char*  : " << buf << std::endl;
    std::cout << "cur   string : " << str << std::endl;

    std::cout << "cur   length : " << bit_stream_3.GetCurrentLength() << std::endl;
    std::cout << "cur   point  : " << bit_stream_3.GetDataPoint() << std::endl;

}