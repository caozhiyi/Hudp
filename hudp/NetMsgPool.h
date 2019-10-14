#ifndef HEADER_HUDP_CNETMSGPOOL
#define HEADER_HUDP_CNETMSGPOOL

#include "CommonFlag.h"
#include "CommonType.h"
#include "NetMsg.h"
#include "TSQueue.h"
#include "Single.h"

namespace hudp {

    class CSenderOrderlyNetMsg; 
    class CSenderRelialeOrderlyNetMsg;
    class CReceiverNetMsg;

    // pool size at initialization.
    //static const uint16_t __netmsg_init_pool_size = 200;
    class CNetMsgPool : public base::CSingle<CNetMsgPool> {
    public:
        CNetMsgPool();
        ~CNetMsgPool();

        // push __init_pool_size net msg to free queue.
        void ExpendFree();
        // reduce half of free queue every time 
        void ReduceFree();

        NetMsg* GetNormalMsg();
        NetMsg* GetSendMsg(uint32_t flag);
        NetMsg* GetRecvMsg();

        void FreeMsg(NetMsg* msg, bool is_recv = false);
        
    private:
        base::CTSQueue<NetMsg*>                         _free_net_msg_queue;
        base::CTSQueue<CSenderOrderlyNetMsg*>           _free_order_queue;
        base::CTSQueue<CSenderRelialeOrderlyNetMsg*>    _free_reliale_order_queue;
        base::CTSQueue<CReceiverNetMsg*>                _free_revceiver_queue;
    };
}
#endif
