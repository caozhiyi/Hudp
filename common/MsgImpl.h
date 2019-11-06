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

        void ClearAck();

        void SetId(const uint16_t& id);
        uint16_t GetId();

        void TranslateFlag();
        void SetFlag(uint16_t flag);
        uint16_t GetFlag();

        void SetHandle(const Handle& handle);
        const Handle& GetHandle();

        void SetBody(const std::string& body);
        std::string& GetBody();

        bool GetSerializeBuffer(char* buf, uint16_t& len);

    private:
        // only head and body will be serialized
        Head            _head;      // head msg. set by hudp
        std::string     _body;      // body msg. set by user
        // other 
        std::string     _ip_port;
        bool            _flag;      // head may be changed, should serialize again. set true
                                    // in recv msg. if send to upper set true

        std::weak_ptr<CSocket> _socket;
        uint64_t       _push_send_time;  // when user send the msg to hudp.
        uint16_t       _backoff_factor;  // timer out resend backoff factor, every time resend it will * 2
    };
}
#endif
