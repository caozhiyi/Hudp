#ifndef HEADER_COMMON_SENDWND
#define HEADER_COMMON_SENDWND

#include <unordered_map>
#include "CommonType.h"
#include "ISendWnd.h"

namespace hudp {

    class CMsg;
    // send window
    class CSendWndImpl : public CSendWnd {
    public:
        CSendWndImpl(uint16_t send_wnd_size);
        ~CSendWndImpl();

        void PushBack(uint16_t id, CMsg* msg);
        // receive a ack
        void AcceptAck(uint16_t id);
        void AcceptAck(uint16_t start_id, uint16_t len);
        void AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len);

        // change send window size
        void ChangeSendWndSize(uint16_t size);

        // remove all msg
        void Clear();



        // add a item to end        
        void PushBack(uint16_t id, CSendWndSolt* data);
        // receive a ack
        void AcceptAck(uint16_t id);
        void AcceptAck(uint16_t start_id, uint16_t len);
        void AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len);

        // change send window size
        void ChangeSendWndSize(uint16_t size);

        // set resend 
        void SetIndexResend(uint16_t id);

        // move item to pool
        void Clear();

    private:
        // call callback func
        void SendAndAck();
        // send next
        void SendNext();

    private:
        CMsg*           _start;       // point to the list start
        CMsg*           _end;         // point to the list end
        CMsg*           _cur;         // current send point
        
        std::mutex      _mutex;
        uint16_t        _send_wnd_size;
        uint16_t        _cur_send_size;

        std::vector<CMsg*>                  _ack_queue;
        std::vector<CMsg*>                  _send_queue;
        std::unordered_map<uint16_t, CMsg*> _id_map;
    }; 
    
}
#endif