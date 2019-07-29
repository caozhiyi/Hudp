#ifdef __linux__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>       // for close
#include <netdb.h>        // for gethostbyname
#include "../OsNet.h"
#include "Log.h"
#include "CommonFunc.h"

using namespace hudp;

bool COsNet::Init() {
    
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

    char hostname[255] = { 0 };
    // get ipv4 
    if (is_ipv4) {
        char name[256] = { 0 };
	    gethostname(name, sizeof(name));
	
	    struct hostent* host = gethostbyname(name);
	    char ip_str[32];
	    const char* ret = inet_ntop(host->h_addrtype, host->h_addr_list[0], ip_str, sizeof(ip_str));
	    if (nullptr == ret) {
		    base::LOG_ERROR("get local ip failed. errno : %d", errno);
		    return "";
	    }
	    return ip_str;
    }

    // struct addrinfo addr_info;
    // memset(&addr_info, 0, sizeof(addr_info));
    // addr_info.ai_family   = AF_INET6;
    // addr_info.ai_socktype = SOCK_DGRAM;
    // addr_info.ai_flags    = AI_PASSIVE;

    // struct addrinfo *into_list;
    // int ret = -1;
    // ret = getaddrinfo(hostname, "", &addr_info, &into_list);
    // if (ret < 0) {
    //     base::LOG_ERROR("get address info failed. error : %d.", errno);
    //     return "";
    // }
    // if(into_list == NULL) {
    //     base::LOG_ERROR("get address info failed. into_list is null.");
    //     return "";
    // }
    // char ipv6[128] = { 0 };
    // int cur_len = 0;
    // // only get first one
    // struct sockaddr_in6 *sinp6;
    // sinp6 = (struct sockaddr_in6 *)into_list->ai_addr;
    // for (int i = 0; i < 16; i++) {
    //     if (((i - 1) % 2) && (i > 0)) {
    //         sprintf(ipv6 + cur_len, ":", sinp6->sin6_addr.u.Byte[i]);
    //         cur_len++;
    //     }

    //     sprintf(ipv6 + cur_len, "%02x", sinp6->sin6_addr.u.Byte[i]);
    //     cur_len += 2;
    // }
    // return std::string(ipv6);
    return "";
}

bool COsNet::Close(uint64_t socket) {
    if (close(socket) <= 0) {
        base::LOG_ERROR("close socket failed. errno %d : ", errno);
        return false;
    }
    return true;
}

#endif