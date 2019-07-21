#ifndef __linux__

#include <Winsock2.h>
#include <ws2tcpip.h>

#include "../OsNet.h"
#include "Log.h"
#include "CommonFunc.h"

#pragma comment(lib,"ws2_32.lib")

using namespace hudp;

static WSADATA __wsa_data;

bool COsNet::Init() {
    if (WSAStartup(MAKEWORD(2, 2), &__wsa_data) != 0) {
        return false;
    }
    return true;
}

void COsNet::Destroy() {
    WSACleanup();
}

bool COsNet::Bind(uint64_t socket, const std::string& ip, uint16_t port) {
    if (port <= 0 || ip.empty()) {
        base::LOG_ERROR("bind socket failed. ip is error. ip : %s", ip.c_str());
        return false;
    }

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

    if (bind(socket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        base::LOG_ERROR("bind socket failed. errno : %d", WSAGetLastError());
        return false;
    }
    return true;
}

int COsNet::SendTo(uint64_t socket, const char * buf, int len, const std::string& ip, uint16_t port) {
    SOCKADDR_IN addr_cli;
    addr_cli.sin_family = AF_INET;
    addr_cli.sin_port = htons(port);
    addr_cli.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    int ret = sendto(socket, buf, len, 0, (SOCKADDR*)&addr_cli, sizeof(SOCKADDR));
    if (ret <= 0) {
        base::LOG_ERROR("send to failed. errno : %d", WSAGetLastError());
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
        base::LOG_ERROR("send to failed. errno : %d", WSAGetLastError());
    }
    return ret;
}

int COsNet::RecvFrom(uint64_t sockfd, char *buf, size_t len, std::string& ip, uint16_t& port) {
    SOCKADDR_IN addr_cli;
    int fromlen = sizeof(SOCKADDR);
    int ret = recvfrom(sockfd, buf, len, 0, (SOCKADDR*)&addr_cli, &fromlen);
    
    if (ret <= 0) {
        base::LOG_ERROR("recv from failed. errno : %d", WSAGetLastError());
        int err = WSAGetLastError();
    }

    ip = inet_ntoa(addr_cli.sin_addr);
    port = ntohs(addr_cli.sin_port);

    return ret;
}
#include <Mstcpip.h>

uint64_t COsNet::UdpSocket() {
    auto ret = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // ignore recvfrom 10054 error
    bool bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(ret, _WSAIOW(IOC_VENDOR, 12), &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);

    if (ret == INVALID_SOCKET) {
        base::LOG_ERROR("get udp socket failed. errno : %d", GetLastError());
        return 0;
    }
    return ret;
}

std::string COsNet::GetOsIp(bool is_ipv4) {

    char hostname[255] = { 0 };
    // get ipv4 
    if (is_ipv4) {
        PHOSTENT hostinfo;
        gethostname(hostname, sizeof(hostname));
        if ((hostinfo = gethostbyname(hostname)) == NULL) {
            base::LOG_ERROR("get host name failed. errno : %d", GetLastError());
            return "";
        }

        char* ip;
        // only get first one
        if(*(hostinfo->h_addr_list) != NULL) {
            ip = inet_ntoa(*(struct in_addr *) *hostinfo->h_addr_list);
            return std::string(ip);
        }
        base::LOG_ERROR("get address info failed. h_addr_list is null.");
        return "";
    }
   
    struct addrinfo addr_info;
    memset(&addr_info, 0, sizeof(addr_info));
    addr_info.ai_family   = AF_INET6;
    addr_info.ai_socktype = SOCK_DGRAM;
    addr_info.ai_flags    = AI_PASSIVE;

    struct addrinfo *into_list;
    int ret = -1;
    ret = getaddrinfo(hostname, "", &addr_info, &into_list);
    if (ret < 0) {
        base::LOG_ERROR("get address info failed. error : %d.", errno);
        return "";
    }
    if(into_list == NULL) {
        base::LOG_ERROR("get address info failed. into_list is null.");
        return "";
    }
    char ipv6[128] = { 0 };
    int cur_len = 0;
    // only get first one
    struct sockaddr_in6 *sinp6;
    sinp6 = (struct sockaddr_in6 *)into_list->ai_addr;
    for (int i = 0; i < 16; i++) {
        if (((i - 1) % 2) && (i > 0)) {
            sprintf(ipv6 + cur_len, ":", sinp6->sin6_addr.u.Byte[i]);
            cur_len++;
        }

        sprintf(ipv6 + cur_len, "%02x", sinp6->sin6_addr.u.Byte[i]);
        cur_len += 2;
    }
    return std::string(ipv6);
}

bool COsNet::Close(uint64_t socket) {
    if (closesocket(socket) == SOCKET_ERROR) {
        base::LOG_ERROR("close socket failed. errno %d : ", WSAGetLastError());
        return false;
    }
    return true;
}

#endif