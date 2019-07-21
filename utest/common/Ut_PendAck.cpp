#include "PendAck.h"
#include "../Test.h"
using namespace hudp;

void UtestPendAck() {
    
    CPendAck pend_ack;

    pend_ack.AddAck(65532);
    pend_ack.AddAck(65533);
    pend_ack.AddAck(65535);
    pend_ack.AddAck(0);

    uint16_t start = 0;
    uint16_t len = 0;

    Expect_True(pend_ack.GetConsecutiveAck(start, len));
    std::cout << "start : " << start << std::endl;
    std::cout << "len   : " << len << std::endl;

    Expect_False(pend_ack.GetConsecutiveAck(start, len));

    pend_ack.AddAck(65534);
    Expect_True(pend_ack.GetConsecutiveAck(start, len));
    std::cout << "start : " << start << std::endl;
    std::cout << "len   : " << len << std::endl;

    pend_ack.AddAck(1);
    pend_ack.AddAck(2);
    pend_ack.AddAck(3);
    pend_ack.AddAck(4);

    std::vector<uint16_t> ack_vec;
    Expect_True(pend_ack.GetAllAck(ack_vec));
    for (auto iter = ack_vec.begin(); iter != ack_vec.end(); ++iter) {
        std::cout << *iter << std::endl;
    }

    Expect_False(pend_ack.GetAllAck(ack_vec));
}