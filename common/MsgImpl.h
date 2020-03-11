#ifndef HEADER_COMMON_MSGIMPL
#define HEADER_COMMON_MSGIMPL

#include <string>   // for string
#include "IMsg.h"
#include "MsgHead.h"

namespace hudp {

    class CSocket;
    class CMsgImpl : public CMsg {
    public:
        CMsgImpl();
        ~CMsgImpl();

        void Clear();
        // clear member which about ack
        void ClearAck();

        void SetId(const uint16_t& id);
        uint16_t GetId();

        // add send delay time
        void AddSendDelay();
        uint16_t GetReSendTime();

        // translate user flag to hudp flag.
        void TranslateFlag();
        void SetHeaderFlag(uint16_t flag);
        uint16_t GetHeaderFlag();

        void SetFlag(uint16_t flag);
        uint16_t GetFlag();

        void SetHandle(const HudpHandle& HudpHandle);
        const HudpHandle& GetHandle();

        void SetBody(const std::string& body);
        std::string& GetBody();

        // ack about
        void SetAck(int16_t flag, std::vector<uint16_t>& ack_vec, bool continuity);
        void GetAck(int16_t flag, std::vector<uint16_t>& ack_vec);

        // get buffer that is serialized
        std::string GetSerializeBuffer();
        bool InitWithBuffer(const std::string& msg);

        // next point about
        void SetNext(CMsg* msg);
        CMsg* GetNext();

        void SetPrev(CMsg* msg);
        CMsg* GetPrev();

        void SetTimerId(uint64_t id);
        uint64_t GetTimerId();

        std::shared_ptr<CSocket> GetSocket();
        void SetSocket(std::shared_ptr<CSocket>& sock);

    private:
        friend class CSerializes;
        // only head and body will be serialized
        Head            _head;      // head msg. set by hudp
        std::string     _body;      // body msg. set by user
        // other 
        std::string     _ip_port;
        int16_t         _flag;      // recv or send message

        CMsg*           _next;
        CMsg*           _prev;

        uint64_t       _time_id;
        uint16_t       _backoff_factor;  // timer out resend backoff factor, every time resend it will * 2

        std::weak_ptr<CSocket> _socket;
    };
}
#endif
