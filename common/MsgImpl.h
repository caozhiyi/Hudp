#ifndef HEADER_COMMON_MSGIMPL
#define HEADER_COMMON_MSGIMPL

#include <string>   // for string

#include "IMsg.h"
#include "MsgHead.h"

namespace hudp {

    class CSocket;
    class CSerializes;
    class CMsgImpl : public CMsg {
    public:
        CMsgImpl();
        ~CMsgImpl();

        void Clear();
        // clear member which about ack
        void ClearAck();

        Head& GetHead();

        void SetId(const uint16_t& id);
        uint16_t GetId();

        // add send delay time
        void AddSendDelay();
        uint16_t GetReSendTime();

        void SetHeaderFlag(uint32_t flag);
        uint32_t GetHeaderFlag();

        void SetFlag(uint32_t flag);
        uint32_t GetFlag();

        // return header flag to string
        std::string DebugHeaderFlag();

        void SetHandle(const HudpHandle& HudpHandle);
        const HudpHandle& GetHandle();

        void SetBody(const std::string& body);
        std::string& GetBody();

        // ack about
        void SetAck(int16_t flag, std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec, bool continuity);
        void GetAck(int16_t flag, std::vector<uint16_t>& ack_vec, std::vector<uint64_t>& time_vec);

        // get buffer that is serialized
        std::string GetSerializeBuffer();
        bool InitWithBuffer(const std::string& msg);

        // next point about
        void SetNext(std::shared_ptr<CMsg> msg);
        std::shared_ptr<CMsg> GetNext();

        void SetPrev(std::shared_ptr<CMsg> msg);
        std::shared_ptr<CMsg> GetPrev();

        void SetTimerId(uint64_t id);
        uint64_t GetTimerId();

        std::shared_ptr<CSocket> GetSocket();
        void SetSocket(std::shared_ptr<CSocket>& sock);

        void SetSendTime(uint64_t time);
        uint64_t GetSendTime();

    private:
        // only head and body will be serialized
        Head            _head;      // head msg. set by hudp
        std::string     _body;      // body msg. set by user
        // other 
        std::string     _ip_port;
        int16_t         _flag;      // recv or send message. only use in self, won't be send

        // two way linked list
        std::shared_ptr<CMsg> _next;
        std::shared_ptr<CMsg> _prev;

        uint64_t       _time_id;
        uint16_t       _backoff_factor;  // timer out resend backoff factor, every time resend it will * 2

        static CSerializes*  _serializes;

        std::weak_ptr<CSocket> _socket;
    };
}
#endif
