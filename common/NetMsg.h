#ifndef HEADER_COMMON_NETMSG
#define HEADER_COMMON_NETMSG
#include <vector>
#include <string>
#include <memory>
#include "CommonType.h"
#include "CommonFlag.h"
namespace hudp {

    class Head {
    public:
        uint16_t _flag;
        uint16_t _id;
        uint16_t _body_len;

        // ack
        uint16_t _ack_len;
        uint16_t _ack_start;
        std::vector<uint16_t> _ack_vec;

        Head() : _flag(0),
                 _id(0),
                 _body_len(0),
                 _ack_len(0),
                 _ack_start(0) {}

        void Clear() {
            _flag = 0;
            _id = 0;
            _body_len = 0;
            _ack_len = 0;
            _ack_start = 0;
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
        process_phase _phase;       // phase in process
        std::weak_ptr<CSocket> _socket;

        NetMsg() {
            memset(_body, 0, __body_size);
        }

        virtual ~NetMsg() {}

        void Clear() {
            _ip_port.clear();
            _head.Clear();
            memset(_body, 0, __body_size);
            _socket.reset();
        }
        void NextPhase() {
            _phase = static_cast<process_phase>(_phase<<1);
        }
    };
}
#endif