#ifndef HEADER_COMMON_SENDWNDSOLT
#define HEADER_COMMON_SENDWNDSOLT

#include "CommonType.h"

namespace hudp {
    
    // send wnd interface, call back when can send a bag
    class CSendWndSolt {
    public:
        CSendWndSolt() : _next(nullptr), _done_ack(false) {}
        virtual ~CSendWndSolt() {}
        
        //*********************
        // to send a bag call back
        //*********************
        virtual void ToSend() = 0;

        //*********************
        // ack call back
        //*********************
        virtual void AckDone() = 0;

        void Clear() {
            _next = nullptr;
            _done_ack = false;
        }

    private:
        friend class CSendWnd;
        CSendWndSolt* _next;
        bool          _done_ack;    // is receive ack?
    };
}
#endif