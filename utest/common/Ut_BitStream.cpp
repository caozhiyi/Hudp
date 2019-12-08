#include "gtest/gtest.h"
#include "BitStream.h"

using namespace hudp;

CBitStreamWriter bit_stream;
CBitStreamReader bit_stream_2;
CBitStreamReader bit_stream_3;
int length_1 = 0;

TEST(BitStream, case1) {
    EXPECT_EQ(400, bit_stream.GetTotalLength());
}

TEST(BitStream, case2) {
    EXPECT_EQ(0, bit_stream.GetCurrentLength());
}

TEST(BitStream, case3) {
    uint16_t uint16 = 12345;
    EXPECT_TRUE(bit_stream.Write(uint16));
    length_1 += sizeof(uint16_t);
}

TEST(BitStream, case4) {
    uint32_t uint32 = 12345678;
    EXPECT_TRUE(bit_stream.Write(uint32));
    length_1 += sizeof(uint32_t);
}

TEST(BitStream, case5) {
    uint64_t uint64 = 12345678910;
    EXPECT_TRUE(bit_stream.Write(uint64));
    length_1 += sizeof(uint64_t);
}
    
TEST(BitStream, case6) {
    char buf[] = "12345678910";
    EXPECT_TRUE(bit_stream.Write(buf, strlen(buf)));
    length_1 += strlen(buf);
}

TEST(BitStream, case7) {
    std::string str = "BasdadasdasdadasB";
    EXPECT_TRUE(bit_stream.Write(str));
    length_1 += str.length();
}
 
TEST(BitStream, case8) {
    EXPECT_EQ(length_1, bit_stream.GetCurrentLength());
}

TEST(BitStream, case9) {
    EXPECT_LE(length_1, bit_stream.GetTotalLength());
}

TEST(BitStream, case10) {
    EXPECT_TRUE(bit_stream_2.Init(bit_stream.GetDataPoint(), bit_stream.GetCurrentLength()));
}

TEST(BitStream, case11) {
    EXPECT_EQ(bit_stream_2.GetCurrentLength(), bit_stream.GetCurrentLength());
}

TEST(BitStream, case12) {
    uint16_t uint16;
    EXPECT_TRUE(bit_stream_2.Read(uint16));
    EXPECT_EQ(12345, uint16);
}

TEST(BitStream, case13) {
    uint32_t uint32 = 12345678;
    EXPECT_TRUE(bit_stream_2.Read(uint32));
    EXPECT_EQ(12345678, uint32);
}

TEST(BitStream, case14) {
    uint64_t uint64 = 12345678910;
    EXPECT_TRUE(bit_stream_2.Read(uint64));
    EXPECT_EQ(12345678910, uint64);
}

TEST(BitStream, case15) {
    char buf[16] = {0};
    EXPECT_TRUE(bit_stream_2.Read(buf, strlen("12345678910")));
    EXPECT_STREQ("12345678910", buf);
}

TEST(BitStream, case16) {
    std::string str;
    EXPECT_TRUE(bit_stream_2.Read(str, strlen("BasdadasdasdadasB")));
    EXPECT_STREQ("BasdadasdasdadasB", str.c_str());
}

TEST(BitStream, case17) {
    EXPECT_TRUE(bit_stream_3.Init(bit_stream_2.GetDataPoint(), bit_stream_2.GetCurrentLength()));
}

TEST(BitStream, case18) {
    EXPECT_EQ(bit_stream_3.GetCurrentLength(), bit_stream_2.GetCurrentLength());
}

TEST(BitStream, case19) {
    uint16_t uint16;
    EXPECT_TRUE(bit_stream_3.Read(uint16));
    EXPECT_EQ(12345, uint16);
}

TEST(BitStream, case20) {
    uint32_t uint32 = 12345678;
    EXPECT_TRUE(bit_stream_3.Read(uint32));
    EXPECT_EQ(12345678, uint32);
}

TEST(BitStream, case21) {
    uint64_t uint64 = 12345678910;
    EXPECT_TRUE(bit_stream_3.Read(uint64));
    EXPECT_EQ(12345678910, uint64);
}

TEST(BitStream, case22) {
    char buf[16] = { 0 };
    EXPECT_TRUE(bit_stream_3.Read(buf, strlen("12345678910")));
    EXPECT_STREQ("12345678910", buf);
}

TEST(BitStream, case23) {
    bit_stream_3.Clear();
    EXPECT_EQ(0, bit_stream_3.GetCurrentLength());
}