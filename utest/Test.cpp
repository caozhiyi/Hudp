#include "gtest/gtest.h"

#ifndef __linux__
#pragma comment(lib,"gtest_maind.lib")
#pragma comment(lib,"gtestd.lib")
#endif // __linux__

int main() {
    testing::InitGoogleTest();
    RUN_ALL_TESTS();
    return 0;
}