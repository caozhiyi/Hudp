#ifndef HEADER_INTERFACE_MSG
#define HEADER_INTERFACE_MSG

#include <vector>
#include <memory>
#include "CommonType.h"

namespace hudp {
    enum flag_type {
        msg_has_changed   = 0x0001,   // message has be change, need serialize again
        msg_is_in_timer   = 0x0002,   // message has be setted in timer
        msg_is_only_ack   = 0x0004,   // message is only with ack
        msg_with_out_id   = 0x0008,   // message don't have id
        msg_send          = 0x0010,   // the msg will be send
        msg_recv          = 0x0020,   // the msg will be recv
        msg_wait_2_msl    = 0x0040    // wait 2 msl time then close
    };

    class CSocket;
    class Head;
    // msg base class.
    class CMsg
    {
    public:
        CMsg() {}
        virtual ~CMsg() {}
        // clear all member 
        virtual void Clear() = 0;
        // // clear member which about ack
        virtual void ClearAck() = 0;

        virtual Head& GetHead() = 0;

        virtual void SetId(const uint16_t& id) = 0;
        virtual uint16_t GetId() = 0;

        // add send delay time
        virtual void AddSendDelay() = 0;
        virtual uint16_t GetReSendTime() = 0;

        virtual void SetHeaderFlag(uint16_t flag) = 0;
        virtual uint16_t GetHeaderFlag() = 0;

        virtual void SetFlag(uint16_t flag) = 0;
        virtual uint16_t GetFlag() = 0;
        
        // return header flag to string
        virtual std::string DebugHeaderFlag() = 0;

        virtual void SetHandle(const HudpHandle& handle) = 0;
        virtual const HudpHandle& GetHandle() = 0;
        
        virtual void SetBody(const std::string& body) = 0;
        virtual std::string& GetBody() = 0;

        // ack about
        virtual void SetAck(int16_t flag, std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool continuity) = 0;
        virtual void GetAck(int16_t flag, std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec) = 0;

        // get buffer that is serialized
        virtual std::string GetSerializeBuffer() = 0;
        virtual bool InitWithBuffer(const std::string&) = 0;

        // next point about
        virtual void SetNext(CMsg* msg) = 0;
        virtual CMsg* GetNext() = 0;

        virtual void SetPrev(CMsg* msg) = 0;
        virtual CMsg* GetPrev() = 0;

        virtual void SetTimerId(uint64_t id) = 0;
        virtual uint64_t GetTimerId() = 0;

        virtual std::shared_ptr<CSocket> GetSocket() = 0;
        virtual void SetSocket(std::shared_ptr<CSocket>& sock) = 0;

        // send time
        virtual void SetSendTime(uint64_t time) = 0;
        virtual uint64_t GetSendTime() = 0;
    };
}
#endif