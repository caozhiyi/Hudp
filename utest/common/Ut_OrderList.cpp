//#include "OrderList.h"
//#include "OrderListSolt.h"
//#include "../Test.h"
//using namespace hudp;
//
//class CTestMsg : public COrderListSolt {
//    public:
//        CTestMsg(uint16_t id) : _id(id) {}
//        ~CTestMsg() {}
//        void ToRecv() {
//            std::cout << "msg id:" << _id << std::endl;
//        }
//        uint16_t Id() { 
//            return _id; 
//        }
//    private:
//        uint16_t _id;
//};
//
//void UtestCOrderListSolt() {
//    
//    CTestMsg msg1(1);
//    CTestMsg msg2(2);
//    CTestMsg msg3(3);
//    CTestMsg msg4(4);
//    CTestMsg msg5(5);
//
//    std::cout << "test CReliableOrderlyList :" << std::endl;
//    CReliableOrderlyList reliable_order_list;
//    reliable_order_list.Insert(msg1.Id(), &msg1);
//    reliable_order_list.Insert(msg2.Id(), &msg2);
//    reliable_order_list.Insert(msg5.Id(), &msg5);
//    reliable_order_list.Insert(msg4.Id(), &msg4);
//    reliable_order_list.Insert(msg3.Id(), &msg3);
//
//    std::cout << "test CReliableList :" << std::endl;
//    CReliableList reliable_list;
//    reliable_list.Insert(msg1.Id(), &msg1);
//    reliable_list.Insert(msg2.Id(), &msg2);
//    reliable_list.Insert(msg5.Id(), &msg5);
//    reliable_list.Insert(msg4.Id(), &msg4);
//    reliable_list.Insert(msg3.Id(), &msg3);
//
//    std::cout << "test COrderlyList :" << std::endl;
//    COrderlyList orderly_list;
//    orderly_list.Insert(msg1.Id(), &msg1);
//    orderly_list.Insert(msg2.Id(), &msg2);
//    orderly_list.Insert(msg5.Id(), &msg5);
//    orderly_list.Insert(msg4.Id(), &msg4);
//    orderly_list.Insert(msg3.Id(), &msg3);
//}