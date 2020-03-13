#ifndef HEADER_NET_OSNET_IMPL
#define HEADER_NET_OSNET_IMPL

#include <iostream>

#include "INetIO.h"
#include "CommonType.h"

namespace hudp {

	class COsNetImpl : public CNetIO {
        public:
        // net library init 
        bool Init();
        void Destroy();

        // bind a socket to ip port
        bool Bind(uint64_t socket, const std::string& ip, uint16_t port);

        // send a msg to client
        int SendTo(uint64_t socket, const char * buf, int len, const std::string& ip, uint16_t port);
        int SendTo(uint64_t socket, const char * buf, int len, const std::string& ip_port);
        int SendTo(uint64_t socket, const char * buf, int len);

        // receiver msg from socket
        int RecvFrom(uint64_t sockfd, char *buf, size_t len, std::string& ip, uint16_t& port);

        // return a udp socket if success. else return 0
        uint64_t UdpSocket();
        
        // get local ip
        std::string GetOsIp(bool is_ipv4 = true);

        // close socket
        bool Close(uint64_t socket);
    };
}

#endif