#ifdef __linux__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>       // for close
#include <netdb.h>        // for gethostbyname  
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h>       // for strlen strcmp

#include "../OsNet.h"
#include "Log.h"
#include "CommonFunc.h"
#include "LinuxFunc.h"

using namespace hudp;

bool COsNet::Init() {
    SetCoreFileUnlimit();
    return true;
}

void COsNet::Destroy() {

}

bool COsNet::Bind(uint64_t socket, const std::string& ip, uint16_t port) {
    if (port <= 0 || ip.empty()) {
        base::LOG_ERROR("bind socket failed. ip is error. ip : %s", ip.c_str());
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (bind(socket, (sockaddr*)&addr, sizeof(addr)) != 0) {
        base::LOG_ERROR("bind socket failed. errno : %d", errno);
        return false;
    }
    return true;
}

int COsNet::SendTo(uint64_t socket, const char * buf, int len, const std::string& ip, uint16_t port) {
    struct sockaddr_in addr_cli;
    addr_cli.sin_family = AF_INET;
    addr_cli.sin_port = htons(port);
    addr_cli.sin_addr.s_addr = inet_addr(ip.c_str());
    int ret = sendto(socket, buf, len, 0, (sockaddr*)&addr_cli, sizeof(addr_cli));
    if (ret <= 0) {
        base::LOG_ERROR("send to failed. errno : %d", errno);
    }
    return ret;
}

int COsNet::SendTo(uint64_t socket, const char * buf, int len, const std::string& ip_port) {
    auto ret = SplitIpPort(ip_port);
    return SendTo(socket, buf, len, ret.second, ret.first);
}

int COsNet::SendTo(uint64_t socket, const char * buf, int len) {
    int ret = send(socket, buf, len, 0);
    if (ret <= 0) {
        base::LOG_ERROR("send to failed. errno : %d", errno);
    }
    return ret;
}

int COsNet::RecvFrom(uint64_t sockfd, char *buf, size_t len, std::string& ip, uint16_t& port) {
    struct sockaddr_in addr_cli;
    socklen_t fromlen = sizeof(sockaddr);
    int ret = recvfrom(sockfd, buf, len, 0, (sockaddr*)&addr_cli, &fromlen);
    
    if (ret <= 0) {
        base::LOG_ERROR("recv from failed. errno : %d", errno);
    }

    ip = inet_ntoa(addr_cli.sin_addr);
    port = ntohs(addr_cli.sin_port);

    return ret;
}

uint64_t COsNet::UdpSocket() {
    auto ret = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (ret <= 0) {
        base::LOG_ERROR("get udp socket failed. errno : %d", errno);
        return 0;
    }
    return ret;
}

std::string COsNet::GetOsIp(bool is_ipv4) {

    struct ifaddrs * if_addr_struct = nullptr;
    void * tmp_addr_ptr = nullptr;

    getifaddrs(&if_addr_struct);

    while (if_addr_struct != nullptr) {
        if (if_addr_struct->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmp_addr_ptr = &((struct sockaddr_in *)if_addr_struct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmp_addr_ptr, addressBuffer, INET_ADDRSTRLEN);
            // get first not local ip
            if (is_ipv4 && strcmp(addressBuffer, "127.0.0.1") != 0) {
                return addressBuffer;
            }

        } else if (if_addr_struct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmp_addr_ptr = &((struct sockaddr_in *)if_addr_struct->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmp_addr_ptr, addressBuffer, INET6_ADDRSTRLEN);

            // get first not local ip
            if (!is_ipv4 && strlen(addressBuffer) > 8) {
                return addressBuffer;
            }
        } 
        if_addr_struct = if_addr_struct->ifa_next;
    }
    return "";
}

bool COsNet::Close(uint64_t socket) {
    if (close(socket) < 0) {
        base::LOG_ERROR("close socket failed. errno %d : ", errno);
        return false;
    }
    return true;
}

#endif