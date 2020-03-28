#include "gtest/gtest.h"
#include "IncrementalId.h"
using namespace hudp;

CIncrementalId id(0);
uint16_t a = 0xFFFF;
uint16_t b = 1;
uint16_t c = 0;

TEST(IncrementalId, case1) {
    EXPECT_TRUE(CIncrementalId::IsNextId(a, c));
}

TEST(IncrementalId, case2) {
    EXPECT_FALSE(CIncrementalId::IsBiggerThan(a, b));
}

TEST(IncrementalId, case3) {
    EXPECT_TRUE(CIncrementalId::IsSmallerThan(a, b));
}