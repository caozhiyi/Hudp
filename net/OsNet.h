#ifndef HEADER_NET_OSNET_INTERFACE
#define HEADER_NET_OSNET_INTERFACE

#include <iostream>
#include "CommonType.h"

namespace hudp {
    class COsNet {
        public:

        // net library init 
        static bool Init();
        static void Destory();

        // bind a socket to ip port
        static bool Bind(uint64_t socket, const std::string& ip, uint16_t port);

        // send a msg to client
        static int SendTo(uint64_t socket, const char * buf, int len, const std::string& ip, uint16_t port);
        static int SendTo(uint64_t socket, const char * buf, int len, const std::string& ip_port);
        static int SendTo(uint64_t socket, const char * buf, int len);

        // receiver msg from socket
        static int RecvFrom(uint64_t sockfd, char *buf, size_t len, std::string& ip, uint16_t& port);

        // return a udp socket if success. else return 0
        static uint64_t UdpSocket();
        
        // get local ip
        static std::string GetOsIp(bool is_ipv4 = true);

        // close socket
        static bool Close(uint64_t socket);
    };
}

#endif