#ifndef HEADER_COMMON_SENDWND
#define HEADER_COMMON_SENDWND

#include <mutex>
#include <unordered_map>
#include "CommonType.h"
#include "SendWndSolt.h"
#include "TSQueue.h"

namespace hudp {
    
    class CNetMsgPool;
    class CBitStreamPool;

    // send window
    class CSendWnd {
    public:
        CSendWnd(uint16_t send_wnd_size);
        ~CSendWnd();
        // add a item to end        
        void PushBack(uint16_t id, CSendWndSolt* data);
        // receive a ack
        void AcceptAck(uint16_t id);
        void AcceptAck(uint16_t start_id, uint16_t len);
        void AcceptAck(std::vector<uint16_t>& vec_id, uint16_t start_index, uint16_t len);

        // change send window size
        void ChangeSendWndSize(uint16_t size);

        // get index of data in window
        CSendWndSolt* GetIndexData(uint16_t id);

        // set resend 
        void SetIndexResend(uint16_t id);

        // move item to pool
        void Clear(CNetMsgPool* msg_pool, CBitStreamPool* bit_pool);

    private:
        // call callback func
        void SendAndAck();
        // send next
        void SendNext();

    private:
        CSendWndSolt* _start;// point to the list start
        CSendWndSolt* _end;  // point to the list end
        
        CSendWndSolt* _cur;  // current send point
        
        std::mutex    _mutex;
        uint16_t      _send_wnd_size;
        uint16_t      _cur_send_size;

        std::unordered_map<uint16_t, CSendWndSolt*> _id_map;

        base::CTSQueue<CSendWndSolt*> _ack_queue;
        base::CTSQueue<CSendWndSolt*> _send_queue;
    }; 
    
}
#endif