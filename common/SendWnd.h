#ifndef HEADER_COMMON_SENDWND
#define HEADER_COMMON_SENDWND

#include <queue>
#include <mutex>
#include <unordered_map>

#include "ISendWnd.h"
#include "CommonType.h"

namespace hudp {

    class CPriorityQueue;
    class CIncrementalId;
    class CMsg;
    // send window
    class CSendWndImpl : public CSendWnd {
    public:
        CSendWndImpl(uint16_t send_wnd_size, CPriorityQueue* priority_queue, bool always_send = false);
        ~CSendWndImpl();

        void PushBack(CMsg* msg);
        // receive a ack
        void AcceptAck(uint16_t id);
        void AcceptAck(uint16_t start_id, uint16_t len);
        void AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len);

        // change send window size
        void ChangeSendWndSize(uint16_t size);

        // remove all msg
        void Clear();

    private:
        // call callback func
        void SendAndAck();
        // send next
        void SendNext();
        // push back to send wnd
        void PushBackToSendWnd(CMsg* msg);      

    private:
        CMsg*           _start;       // point to the list start
        CMsg*           _end;         // point to the list end
        CMsg*           _cur;         // current send point
        
        std::mutex      _mutex;
        uint16_t        _send_wnd_size;
        uint16_t        _cur_send_size;
        bool            _always_send;

        CPriorityQueue* _priority_queue;
        CIncrementalId* _incremental_id;

        std::queue<CMsg*>                   _ack_queue;
        std::queue<CMsg*>                   _send_queue;
        std::unordered_map<uint16_t, CMsg*> _id_msg_map;
    }; 
    
}
#endif