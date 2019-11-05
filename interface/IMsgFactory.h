#ifndef HEADER_INTERFACE_MSGFACTORY
#define HEADER_INTERFACE_MSGFACTORY

#include "CommonType.h"

namespace hudp {
    class CMsg;
    class CMsgFactory
    {
    public:
        CMsgFactory() {}
        virtual ~CMsgFactory() {}

        virtual CMsg* CreateMsg(uint16_t flag) = 0;

        virtual void DeleteMsg(CMsg* msg) = 0;
    };
}
#endif