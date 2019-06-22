#ifndef HEADER_COMMON_BITSTREAM
#define HEADER_COMMON_BITSTREAM

#include <mutex>    // for mutex
#include <string>   // for string
#include "CommonType.h"
namespace hudp {

    static const bool     __must_less_mtu  = false;
    static const uint16_t __max_length     = (uint16_t)0xFFFF;

    class CHudpBitStream {
    public:
        CHudpBitStream();
        ~CHudpBitStream();

        // get length interface
        uint16_t GetTotalLength() const;
        uint16_t GetCurrentLength() const;

        const char* GetDataPoint() const;

        void Clear();
        
        // only can write type that defined in common type
        template<typename T>
        bool Write(T& value);
        bool Write(const char* value, uint16_t len);
        bool Write(const std::string& value);
        bool Write(const CHudpBitStream& value);

        template<typename T>
        bool Read(T& value);
        bool Read(char* value, uint16_t len);
        bool Read(std::string& value, uint16_t len);
        bool Read(CHudpBitStream& value, uint16_t len);

    private:
        // check the left over buffer is enough?
        bool CheckBufferExpend(uint16_t len);
        void CopyMemory(void* value, uint16_t len);

    private:
        std::mutex  _mutex;
        char*       _data;       // date buffer
        char*       _cur_point;  // current read/write point
        // max support 65535 length.
        uint16_t    _length;
        uint16_t    _cur_length;
    };

    template<typename T>
    bool CHudpBitStream::Write(T& value) {
        uint16_t len = sizeof(value);
        
        if (!CheckBufferExpend(len)) {
            return false;
        }
        // write value
        memcpy(_cur_point, (void*)&value, len);
        _cur_point += len;
        _cur_length += len;
        return true;
    }

    template<typename T>
    bool CHudpBitStream::Read(T& value) {
        uint16_t len = sizeof(value);
        if (_cur_length + len > _length) {
            return false;
        }
        memcpy((void*)&value, _cur_point, len);
        _cur_point += len;
        _cur_length += len;
        return true;
    }
}

#endif