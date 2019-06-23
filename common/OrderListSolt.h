#ifndef HEADER_COMMON_ORDERLISTSOLT
#define HEADER_COMMON_ORDERLISTSOLT

#include <mutex>
#include "CommonType.h"

namespace hudp {
    
    // if care about receiving bag, inherit that it
    class COrderListSolt {
    public:
        COrderListSolt() {}
        ~COrderListSolt() {}

        //*********************
        // notify upper receive a bag 
        //*********************
        virtual void ToRecv() = 0;

    //    bool GetDoneBack() {
    //        return _done_ack;
    //    }
    //    void SetDoneBack(bool done_ack) {
    //        _done_ack = done_ack;
    //    }

    //private:
    //    bool _done_ack;
    }; 
}
#endif