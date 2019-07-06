#include "Serializes.h"
#include "../Test.h"
#include "NetMsg.h"
#include "BitStream.h"
#include "CommonFlag.h"

using namespace hudp;

void UtestCSerializes() {
    NetMsg msg1;
    NetMsg msg2;
    NetMsg msg3;
    NetMsg msg4;
    char buf[] = "it is a test body";
    uint16_t len = strlen(buf);
    memcpy(msg1._body, buf, len);
    msg1._head._flag |= HPF_LOW_PRI;
    msg1._head._flag |= HPF_WITH_ID;
    msg1._head._id = 1201;

    CBitStreamWriter wrtier_stream;
    CBitStreamReader reader_stream;

    Expect_True(CSerializes::Serializes(msg1._head, wrtier_stream));
    reader_stream.Init(wrtier_stream.GetDataPoint(), wrtier_stream.GetCurrentLength());
    Expect_True(CSerializes::Deseriali(reader_stream, msg2._head));

    wrtier_stream.Clear();
    reader_stream.Clear();
    Expect_True(CSerializes::Serializes(msg1, wrtier_stream));
    reader_stream.Init(wrtier_stream.GetDataPoint(), wrtier_stream.GetCurrentLength());
    Expect_True(CSerializes::Deseriali(reader_stream, msg3));

    wrtier_stream.Clear();
    reader_stream.Clear();
    Expect_True(CSerializes::Serializes(msg1._head, "test msg bosy", strlen("test msg bosy"), wrtier_stream));
    reader_stream.Init(wrtier_stream.GetDataPoint(), wrtier_stream.GetCurrentLength());
    char buf_recv[128] = {0};
    uint16_t len_recv = 0;
    Expect_True(CSerializes::Deseriali(reader_stream, msg4._head, buf_recv, len_recv));
}