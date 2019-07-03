#ifndef HEADER_COMMON_PRIORITYQUEUE
#define HEADER_COMMON_PRIORITYQUEUE

#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "CommonType.h"
#include "NetMsg.h"
#include "TSQueue.h"
namespace hudp {
    
    // we have four priority level
    static const uint16_t __pri_queue_size = 4;
    // send __pri_surplus high-level packages and one low-level package when busy
    static const uint8_t  __pri_surplus    = 2;

    static const uint16_t __pri_low        = 3;
    static const uint16_t __pri_normal     = 2;
    static const uint16_t __pri_heig       = 1;
    static const uint16_t __pri_heighest   = 0;
    
    class CPriorityQueue {
    public:   
        CPriorityQueue();
        ~CPriorityQueue();

        void Push(NetMsg* msg);

        // get a item from queue by priority
        NetMsg* Pop();

        // get current number of item in queue
        uint64_t Size();

        // clear all item
        void Clear();
        
    private:
        base::CTSQueue<NetMsg*>     _queue_arr[__pri_queue_size];
        base::CTSQueue<NetMsg*>*    _cur_queue;

        std::mutex				    _mutex;
        std::condition_variable_any	_notify;
        std::atomic<uint64_t>       _size;

        // control priority surplus
        int8_t                      _pri_normal_count;
        int8_t                      _pri_heig_count;
        int8_t                      _pri_heighest_count;
    };
}
#endif