#ifndef HEADER_COMMON_MSGIMPL
#define HEADER_COMMON_MSGIMPL

#include <string>   // for string
#include "IMsg.h"
#include "MsgHead.h"

namespace hudp {

    class CSocket;
    class CMsgImpl : public CMsg {
    public:
        CMsgImpl(uint16_t flag);
        ~CMsgImpl();

        void TranslateFlag();
        void SetHandle(const Handle& handle);
        const Handle& GetHandle();

        void Clear();

        void ClearAck();

        void SetId(const uint16_t& id);

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
