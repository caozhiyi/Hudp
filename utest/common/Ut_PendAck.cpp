#include "gtest/gtest.h"
#include "PendAck.h"
using namespace hudp;

CPendAck pend_ack;

TEST(PendAck, case1) {

    pend_ack.AddAck(65532);
    pend_ack.AddAck(65533);
    pend_ack.AddAck(65534);
    pend_ack.AddAck(65535);
    pend_ack.AddAck(0);

    EXPECT_TRUE(pend_ack.HasAck());
}

std::vector<uint16_t> vec;
bool con = false;

TEST(PendAck, case2) {
    EXPECT_TRUE(pend_ack.GetAllAck(vec, con));
}

TEST(PendAck, case3) {
    EXPECT_EQ(65532, vec[0]);
}

TEST(PendAck, case4) {
    EXPECT_EQ(5, vec.size());
}

TEST(PendAck, case5) {
    EXPECT_TRUE(con);
}