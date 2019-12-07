//#include "PendAck.h"
//#include "../Test.h"
//using namespace hudp;
//
//void UtestPendAck() {
//    
//    CPendAck pend_ack;
//
//    pend_ack.AddAck(65532);
//    pend_ack.AddAck(65533);
//    pend_ack.AddAck(65534);
//    pend_ack.AddAck(65535);
//    pend_ack.AddAck(0);
//
//    uint16_t start = 0;
//    uint16_t len = 0;
//
//    Expect_True(pend_ack.HasAck());
//    std::vector<uint16_t> vec;
//    bool con = false;
//    pend_ack.GetAllAck(vec, con);
//    std::cout << "start : " << vec[0] << std::endl;
//    std::cout << "size  : " << vec.size() << std::endl;
//    std::cout << "con   : " << con << std::endl;
//
//    pend_ack.AddAck(1);
//    pend_ack.AddAck(2);
//    pend_ack.AddAck(3);
//    pend_ack.AddAck(4);
//    pend_ack.AddAck(5);
//
//    vec.clear();
//    Expect_True(pend_ack.HasAck());
//    pend_ack.GetAllAck(vec, con);
//    std::cout << "start : " << vec[0] << std::endl;
//    std::cout << "size  : " << vec.size() << std::endl;
//    std::cout << "con   : " << con << std::endl;
//
//}