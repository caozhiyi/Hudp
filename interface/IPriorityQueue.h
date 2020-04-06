#ifndef HEADER_INTERFACE_PRIORITYQUEUE
#define HEADER_INTERFACE_PRIORITYQUEUE

#include <memory> // for shared_ptr
#include "CommonType.h"

namespace hudp {

    class CMsg;
    class CPriorityQueue {
    public:
        CPriorityQueue() {}
        virtual ~CPriorityQueue() {}

        virtual void PushBack(std::shared_ptr<CMsg> msg) = 0;

        // get a item from queue by priority
        virtual std::shared_ptr<CMsg> Pop() = 0;

        // get current number of item in queue
        virtual uint64_t Size() = 0;

        // clear all item
        virtual void Clear() = 0;
    };
}
#endif