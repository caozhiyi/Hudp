#ifndef HEADER_INTERFACE_NETIO
#define HEADER_INTERFACE_NETIO

#include "CommonType.h"

namespace hudp {
    // msg base class.
    class CNetIO
    {
    public:
        CNetIO() {}
        virtual ~CNetIO() {}

        // net library init 
        virtual bool Init() = 0;
        virtual void Destroy() = 0;

        // bind a socket to ip port
        virtual bool Bind(uint64_t socket, const std::string& ip, uint16_t port) = 0;

        // send a msg to client
        virtual int SendTo(uint64_t socket, const char * buf, int len, const std::string& ip, uint16_t port) = 0;
        virtual int SendTo(uint64_t socket, const char * buf, int len, const std::string& ip_port) = 0;

        // receiver msg from socket
        virtual int RecvFrom(uint64_t sockfd, char *buf, size_t len, std::string& ip, uint16_t& port) = 0;

        // return a udp socket if success. else return 0
        virtual uint64_t UdpSocket() = 0;

        // close socket
        virtual bool Close(uint64_t socket) = 0;
    };
}
#endif