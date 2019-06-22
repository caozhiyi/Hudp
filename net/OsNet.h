#ifndef HEADER_OSNET_INTERFACE
#define HEADER_OSNET_INTERFACE

#include "CommonType.h"

namespace hudp {
    struct sockaddr;
    class COsNet {
        
        // os sentto api
        static int SendTo(uint64_t socket, const char * buf, int len, 
                    int flags, const struct sockaddr *to, int tolen);

        // os recvfrom api
        static int RecvFrom(uint64_t sockfd, void *buf, size_t len, 
                    unsigned int flags, struct sockaddr *from, int *fromlen);
        
    };
}

#endif