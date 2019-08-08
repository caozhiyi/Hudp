#include <string.h>  // for strlen
#include "OsNet.h"
#include "../Test.h"

using namespace hudp;

void UtestOsNet() {

    Expect_True(COsNet::Init());

    auto ipv4 = COsNet::GetOsIp();
    std::cout << "ipv4 : " << ipv4.c_str() << std::endl;
    auto ipv6 = COsNet::GetOsIp(false);
    std::cout << "ipv6 : " << ipv6.c_str() << std::endl;

    uint64_t socket1 = COsNet::UdpSocket();
    std::cout << "socket1 : " << socket1 << std::endl;
    Expect_Bigger(socket1, 0);
    
    Expect_True(COsNet::Bind(socket1, ipv4, 4396));

    uint64_t socket2 = COsNet::UdpSocket();
    std::cout << "socket2 : " << socket2 << std::endl;
    Expect_Bigger(socket2, 0);

    Expect_True(COsNet::Bind(socket2, ipv4, 4397));
    int ret = COsNet::SendTo(socket2, "test msg", strlen("test msg"), ipv4, 4396);
    std::cout << "SendTo : " << ret << std::endl;
    Expect_Bigger(ret, 0);

    char buf[128] = {};
    std::string from_ip;
    uint16_t port = 0;
    ret = COsNet::RecvFrom(socket1, buf, 128, from_ip, port);
    std::cout << "RecvFrom : " << ret << std::endl;
    Expect_Bigger(ret, 0);

    Expect_True(COsNet::Close(socket1));
    Expect_True(COsNet::Close(socket2));
}