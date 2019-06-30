#ifndef HEADER_OSNET_INTERFACE
#define HEADER_OSNET_INTERFACE

#include "CommonType.h"

namespace hudp {
    static const uint8_t __ip_length = 32;
    class CHudpSocket {
        
    

    private:
        char            _ip[__ip_length];
        uint8_t         _short;
        unsigned int	_sock;
    };
}

#endif