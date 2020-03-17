#include <memory>
#include <iostream>
#include "gtest/gtest.h"
#include "IOrderList.h"
#include "OrderListImpl.h"
#include "MsgImpl.h"
#include "SocketImpl.h"

using namespace hudp;

uint64_t global_id = 0;

class CTestSocket : public CSocketImpl {
public:
    CTestSocket() : CSocketImpl("") {}
    ~CTestSocket() {}
    void ToRecv(CMsg* msg) {
        global_id = msg->GetId();
    }
};

std::shared_ptr<CSocket> sock(new CTestSocket());

std::shared_ptr<CMsg> msg1(new CMsgImpl());
std::shared_ptr<CMsg> msg2(new CMsgImpl());
std::shared_ptr<CMsg> msg3(new CMsgImpl());
std::shared_ptr<CMsg> msg4(new CMsgImpl());
std::shared_ptr<CMsg> msg5(new CMsgImpl());

TEST(OrderList, case1) {
    msg1->SetId(1);
    msg2->SetId(2);
    msg3->SetId(3);
    msg4->SetId(4);
    msg5->SetId(5);

    msg1->SetSocket(sock);
    msg2->SetSocket(sock);
    msg3->SetSocket(sock);
    msg4->SetSocket(sock);
    msg5->SetSocket(sock);

    CReliableOrderlyList reliable_order_list(1);
    reliable_order_list.Insert(msg1.get());
    EXPECT_EQ(1, global_id);
    reliable_order_list.Insert(msg2.get());
    EXPECT_EQ(2, global_id);
    reliable_order_list.Insert(msg5.get());
    EXPECT_EQ(2, global_id);
    reliable_order_list.Insert(msg4.get());
    EXPECT_EQ(2, global_id);
    reliable_order_list.Insert(msg3.get());
    EXPECT_EQ(5, global_id);
}

TEST(OrderList, case2) {
    CReliableList reliable_list(1);
    reliable_list.Insert(msg1.get());
    EXPECT_EQ(1, global_id);
    reliable_list.Insert(msg2.get());
    EXPECT_EQ(2, global_id);
    reliable_list.Insert(msg5.get());
    EXPECT_EQ(5, global_id);
    reliable_list.Insert(msg4.get());
    EXPECT_EQ(4, global_id);
    reliable_list.Insert(msg3.get());
    EXPECT_EQ(3, global_id);
}

TEST(OrderList, case3) {
    COrderlyList orderly_list(1);
    orderly_list.Insert(msg1.get());
    EXPECT_EQ(1, global_id);
    orderly_list.Insert(msg2.get());
    EXPECT_EQ(2, global_id);
    orderly_list.Insert(msg5.get());
    EXPECT_EQ(5, global_id);
    orderly_list.Insert(msg4.get());
    EXPECT_EQ(5, global_id);
    orderly_list.Insert(msg3.get());
    EXPECT_EQ(5, global_id);
}