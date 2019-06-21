#ifndef HEADER_CBITSTREAM
#define HEADER_CBITSTREAM

#include <mutex>
#include <assert.h>
#include "CommonType.h"
static const bool __must_less_mtu  = false;
static const uint16_t __max_length = (uint16_t)65536;
namespace hudp {
    class CHudpBitStream {
    public:
        CHudpBitStream();
        ~CHudpBitStream();

        // get length interface
        uint16_t GetTotalLength() const;
        uint16_t GetCurrentLength() const;

        const char* GetDataPoint() const;
        
        // only can write type that defined in common type
        template<typename T>
        bool Write(T& value);
        bool Write(char* value, uint16_t len);
        bool Write(const std::string& value);
        bool Write(const CHudpBitStream& value);
    
    private:
        // check the left over buffer is enough?
        bool CheckBufferExpend(uint16_t len);
        void CopyMemory(void* value, uint16_t len);

    private:
        std::mutex  _mutex;
        char*       _data;       // date buffer
        char*       _cur_point;  // current read/write point
        // max support 65536 length.
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
        memccpy(_cur_point, (void*)value, len);
        _cur_point += len;
        _cur_length += len;
        return true;
    }
}

#endif