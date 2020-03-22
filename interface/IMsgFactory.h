#ifndef HEADER_INTERFACE_MSGFACTORY
#define HEADER_INTERFACE_MSGFACTORY

#include <memory>
#include "CommonType.h"

namespace hudp {
    class CMsg;
    class CMsgFactory
    {
    public:
        CMsgFactory() {}
        virtual ~CMsgFactory() {}

        virtual std::shared_ptr<CMsg> CreateSharedMsg() = 0;

        virtual CMsg* CreateMsg() = 0;

        virtual void DeleteMsg(CMsg* msg) = 0;
    };
}
#endif