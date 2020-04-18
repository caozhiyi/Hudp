#ifndef HEADER_NET_CONTROLLER_FLOWQUEUE
#define HEADER_NET_CONTROLLER_FLOWQUEUE

#include <list>
#include <mutex>
#include <memory>

#include "CommonFlag.h"

namespace hudp {

    class CMsg;

    /* add msg to flow queue
     * flow queue is a linked list, kind of FIFO, except for TCP retransmits
     * head->  [retrans pkt 1]
     *         [retrans pkt 2]
     *         [ normal pkt 1]
     *         [ normal pkt 2]
     *         [ normal pkt 3]
     * tail->  [ normal pkt 4]
     */

    class CFlowQueue {
    public:
        // add msg to list. if msg is resend, will add to list head
        void Add(std::shared_ptr<CMsg> msg);
        // get msg from head and remove it from list
        std::shared_ptr<CMsg> Get();
        // remove msg from list any where
        void Remove(std::shared_ptr<CMsg> msg);
    private:
        // remove not lock
        void RemoveUnLock(std::shared_ptr<CMsg> msg);
        // add msg to normal list head
        void AddToNormalHead(std::shared_ptr<CMsg> msg);
        // add msg to normal list tail
        void AddToNormalTail(std::shared_ptr<CMsg> msg);
        // add msg to resend list head
        void AddToResendHead(std::shared_ptr<CMsg> msg);
        // add msg to resend list tail
        void AddToResendTail(std::shared_ptr<CMsg> msg);

    private:
        std::mutex            _normal_mutex;
        std::mutex            _resend_mutex;
        std::list<std::shared_ptr<CMsg>>    _resend_list;
        std::list<std::shared_ptr<CMsg>>    _normal_list;
    };
}
#endif