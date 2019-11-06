#ifndef HEADER_COMMON_PRIORITYQUEUE
#define HEADER_COMMON_PRIORITYQUEUE

#include <queue> // for queue

#include "CommonType.h"
#include "IMsg.h"
#include "IPriorityQueue.h"

namespace hudp {
    
    // we have four priority level
    static const uint16_t __pri_queue_size = 4;

    static const uint16_t __pri_low        = 3;
    static const uint16_t __pri_normal     = 2;
    static const uint16_t __pri_high       = 1;
    static const uint16_t __pri_highest    = 0;
    
    class CPriorityQueueImpl : public CPriorityQueue {
    public:   
        CPriorityQueueImpl();
        ~CPriorityQueueImpl();

        void Push(CMsg* msg);

        // get a item from queue by priority
        CMsg* Pop();

        // get current number of item in queue
        uint64_t Size();

        // clear all item
        void Clear();
        
    private:
        std::queue<CMsg*>           _queue_arr[__pri_queue_size];
        // control priority surplus
        int8_t                      _pri_normal_count;
        int8_t                      _pri_high_count;
        int8_t                      _pri_highest_count;
    };
}
#endif
