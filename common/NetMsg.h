#ifndef HEADER_COMMON_NETMSG
#define HEADER_COMMON_NETMSG
#include <vector>
#include <string>
#include "CommonType.h"
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
    

    class CBitStream;
    class NetMsg {
    public:
        // only head and body will be serialized
        Head        _head;        // head msg. set by hudp
        char*       _body;        // body msg. set by user

        // other 
        std::string _ip_port;
        CBitStream* _bit_stream;  // serialize stream

        NetMsg() : _body(nullptr) {}

        virtual ~NetMsg() {}

        void Clear() {
            _ip_port.clear();
            _head.Clear();
            _body = nullptr;
        }
    };
}
#endif