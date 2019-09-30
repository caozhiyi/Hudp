#ifndef HEADER_COMMON_NETMSG
#define HEADER_COMMON_NETMSG
#include <vector>
#include <string>
#include <memory>
#include <cstring>		//for memset

#include "CommonType.h"
#include "CommonFlag.h"
namespace hudp {

    class Head {
    public:
        uint16_t _flag;
        uint16_t _id;
        uint16_t _body_len;
 
        // ack
        uint16_t _ack_reliable_len;
        uint16_t _ack_reliable_orderly_len;
        std::vector<uint16_t> _ack_vec;

        Head() : _flag(0),
                 _id(0),
                 _body_len(0),
                 _ack_reliable_len(0),
                 _ack_reliable_orderly_len(0) {}

        void Clear() {
            _flag = 0;
            _id = 0;
            _body_len = 0;
            _ack_reliable_len = 0;
            _ack_reliable_orderly_len = 0;
            _ack_vec.clear();
        }
    };
    
    // this size better less than mtu
    static const uint16_t __body_size = __mtu;

    class CBitStream;
    class CSocket;
    class NetMsg {
    public:
        // only head and body will be serialized
        Head        _head;        // head msg. set by hudp
        char        _body[__body_size];        // body msg. set by user

        // other 
        std::string   _ip_port;
        CBitStream*   _bit_stream;  // serialize stream
        uint8_t       _phase;       // phase in process
        std::weak_ptr<CSocket> _socket;

        uint16_t      _backoff_factor;  // timer out resend backoff factor
        bool          _re_send;// is resend?
        bool          _flag;   // head may be changes, should serialize again. set true
                               // in recv msg. if send to upper set true

        bool          _use;    // check msg is used

        NetMsg() : _backoff_factor(1),
                   _re_send(false),
                   _flag(false),
                   _use(true) {
            memset(_body, 0, __body_size);
        }

        virtual ~NetMsg() {}

        void Clear() {
            _re_send = false;
            _flag = false;
            _ip_port.clear();
            _head.Clear();
            memset(_body, 0, __body_size);
            _socket.reset();
            _bit_stream = nullptr;
            _use = false;
            _backoff_factor = 1;
        }

        void ClearAck() {
            _head._ack_reliable_len = 0;
            _head._ack_reliable_orderly_len = 0;
            _head._ack_vec.clear();
            _head._flag &= ~HPF_WITH_RELIABLE_ACK;
            _head._flag &= ~HPF_WITH_RELIABLE_ORDERLY_ACK;
            _head._flag &= ~HPF_RELIABLE_ACK_RANGE;
            _head._flag &= ~HPF_RELIABLE_ORDERLY_ACK_RANGE;
        }

        void SetId(const uint16_t& id) {
            _head._id = id;
            _head._flag |= HPF_WITH_ID;
        }
    };
}
#endif
