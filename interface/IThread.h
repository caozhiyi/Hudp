#ifndef HEADER_INTERFACE_THREAD
#define HEADER_INTERFACE_THREAD

#include <functional>
#include "CommonType.h"

namespace hudp {
    // thread base class.
    class CThread
    {
    public:
        CThread() {}
        virtual ~CThread() {}
        // add a func to thread.
        // all func while be called on turn by index.
        virtual bool AddRunFunc(const std::function<void(void)>& func, int16_t index) = 0;

        virtual void Start() = 0;

        virtual void Stop() = 0;

        virtual void Join() = 0;
    };
}
#endif