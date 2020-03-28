#include "MsgImpl.h"
#include "BitStream.h"
#include "Serializes.h"
#include "CommonFlag.h"
#include "gtest/gtest.h"

using namespace hudp;

CMsgImpl msg1;
CMsgImpl msg2;
CBitStreamWriter wrtier_stream;
CBitStreamReader reader_stream;
CSerializesNormal serlialize;

TEST(Serializes, case1) {

    msg1.SetBody("it is a test body");
    msg1.SetHeaderFlag(HPF_LOW_PRI | HPF_WITH_ID);
    msg1.SetId(1201);

    EXPECT_TRUE(serlialize.Serializes(msg1, wrtier_stream));
}

TEST(Serializes, case2) {

    reader_stream.Init(wrtier_stream.GetDataPoint(), wrtier_stream.GetCurrentLength());
    EXPECT_TRUE(serlialize.Deseriali(reader_stream, msg2));
}

TEST(Serializes, case3) {
    
    EXPECT_TRUE(msg1.GetBody() == msg2.GetBody());
    EXPECT_TRUE(msg1.GetId() == msg2.GetId());
    EXPECT_TRUE(msg1.GetHeaderFlag() == msg2.GetHeaderFlag());
}

TEST(Serializes, case4) {

    EXPECT_TRUE(wrtier_stream.GetCurrentLength() == serlialize.EstimateSize(msg2));
}