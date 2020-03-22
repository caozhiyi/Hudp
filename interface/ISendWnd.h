#ifndef HEADER_INTERFACE_SENDWND
#define HEADER_INTERFACE_SENDWND

#include <memory> // for shared_ptr
#include <vector> // for vector
#include "CommonType.h"

namespace hudp {
    class CMsg;
    class CSendWnd
    {
    public:
        CSendWnd() {}
        virtual ~CSendWnd() {}

        virtual void PushBack(std::shared_ptr<CMsg> msg) = 0;
        // receive a ack
        virtual void AcceptAck(uint16_t id) = 0;
        virtual void AcceptAck(uint16_t start_id, uint16_t len) = 0;
        virtual void AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len) = 0;

        // change send window size
        virtual void ChangeSendWndSize(uint16_t size) = 0;

        // remove all msg
        virtual void Clear() = 0;
    };
}
#endif