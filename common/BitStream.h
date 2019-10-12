#ifndef HEADER_COMMON_BITSTREAM
#define HEADER_COMMON_BITSTREAM

#include <mutex>    // for mutex
#include <string>   // for string
#include <string.h> // for memset
#include "CommonType.h"
#include "Single.h"
namespace hudp {

    static const bool     __must_less_mtu  = false;
    static const uint16_t __max_length     = (uint16_t)0xFFFF;

    // only can read or write
    class CBitStream {
    public:
        CBitStream();
        virtual ~CBitStream();

        // get all cache length
        uint16_t GetTotalLength();
        // get current data length
        uint16_t GetCurrentLength();

        // set init data, only copy data to cache.
        // only can be called once
        bool Init(const char* value, uint16_t len);

        // get source point to data
        const char* GetDataPoint();

        void Clear();
        
    protected:
        // check the left over buffer is enough?
        bool CheckBufferExpend(uint16_t len);
        void CopyMemory(void* value, uint16_t len);

    protected:
        std::mutex  _mutex;
        char*       _data;       // date buffer
        char*       _cur_point;  // current read/write point
        // max support 65535 length.
        uint16_t    _length;
        uint16_t    _cur_length;
    };

    class CBitStreamReader : public CBitStream {
    public:
        CBitStreamReader() {}
        virtual ~CBitStreamReader() {}

        template<typename T>
        bool Read(T& value);
        bool Read(char* value, uint16_t len);
        bool Read(std::string& value, uint16_t len);
        bool Read(CBitStreamReader& value, uint16_t len);
    };

    class CBitStreamWriter : public CBitStream {
    public:
        CBitStreamWriter() {}
        virtual ~CBitStreamWriter() {}

        // only can write type that defined in common type
        template<typename T>
        bool Write(const T& value);
        bool Write(const char* value, uint16_t len);
        bool Write(const std::string& value);
        bool Write(CBitStreamWriter& value);
    };

    template<typename T>
    bool CBitStreamWriter::Write(const T& value) {
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
    bool CBitStreamReader::Read(T& value) {
        uint16_t len = sizeof(value);
        if (_cur_point - _data + len > _cur_length) {
            return false;
        }
        memcpy((void*)&value, _cur_point, len);
        _cur_point += len;
        return true;
    }
}

#endif
