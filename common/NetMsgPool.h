#ifndef HEADER_COMMON_CNETMSGPOOL
#define HEADER_COMMON_CNETMSGPOOL

#include "CommonType.h"
#include "Common.h"
#include "TSQueue.h"

namespace hudp {
    
    // pool size at initialization.
    static const uint16_t __init_pool_size = 200;
    class CNetMsgPool {
        
        CNetMsgPool();
        ~CNetMsgPool();

        // push __init_pool_size net msg to free queue.
        void ExpendFree();
        // reduce half of free queue every time 
        void ReduceFree();

        NetMsg* GetMsg();

        void FreeMsg(NetMsg* msg);
        
    private:
        base::CTSQueue<NetMsg*>     _free_queue;
    };
}
#endif