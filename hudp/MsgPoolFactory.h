#ifndef HEADER_HUDP_CNETMSGPOOL
#define HEADER_HUDP_CNETMSGPOOL

#include "TSQueue.h"
#include "CommonType.h"
#include "IMsgFactory.h"

namespace hudp {

    // pool size at initialization.
    // static const uint16_t __netmsg_init_pool_size = 200;
    class CMsg;
    class CMsgPoolFactory : public CMsgFactory {
    public:
        CMsgPoolFactory();
        ~CMsgPoolFactory();

        CMsg* CreateMsg();

        void DeleteMsg(CMsg* msg);
    private:
        // reduce half of free queue every time 
        void ReduceFree();
        
    private:
        base::CTSQueue<CMsg*>  _free_net_msg_queue;
    };
}
#endif
