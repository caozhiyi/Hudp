#include <memory.h> // for memset memcpy
#include "BitStream.h"
#include "Common.h"
#include "Log.h"

using namespace hudp;

CHudpBitStream::CHudpBitStream() : _data(nullptr),
                                   _cur_point(nullptr),
                                   _length(__mtu),
                                   _cur_length(0) {
    _data = new char[__mtu];
    memset(_data, 0, __mtu);
}

CHudpBitStream::~CHudpBitStream() {
    if (_data) {
        delete _data;
    }
}

uint16_t CHudpBitStream::GetTotalLength() const {
    return _length;
}

uint16_t CHudpBitStream::GetCurrentLength() const {
    return _cur_length;
}

const char* CHudpBitStream::GetDataPoint() const {
    return _data;
}

bool CHudpBitStream::CheckBufferExpend(uint16_t len) {
    if (__max_length - _cur_length < len) {
        base::LOG_ERROR("write stream more than max length.");
        return false;
    }
    if (_cur_length + len > _length) {
        if (__must_less_mtu) {
            base::LOG_ERROR("write stream more than mtu length.");
            return false;
        }

        // expand data buffer
        _length *= 2;
        char* temp_data = new char[_length];
        memset(temp_data, 0, _length);
        memcpy(temp_data, _data, _cur_length);
        delete _data;
        _data = temp_data;
    }
    return true;
}

void CHudpBitStream::CopyMemory(void* value, uint16_t len) {
    memcpy(_cur_point, (void*)value, len);
    _cur_point += len;
    _cur_length += len;
}

bool CHudpBitStream::Write(const char* value, uint16_t len) {
    if (!CheckBufferExpend(len)) {
        return false;
    }
    // write value
    CopyMemory((void*)value, len);
    return true;
}

bool CHudpBitStream::Write(const std::string& value) {
    uint16_t len = (uint16_t)value.length();
    if (!CheckBufferExpend(len)) {
        return false;
    }
    // write value
    CopyMemory((void*)value.c_str(), len);
    return true;
}

bool CHudpBitStream::Write(const CHudpBitStream& value) {
    uint16_t len = value.GetCurrentLength();
    if (!CheckBufferExpend(len)) {
        return false;
    }
    // write value
    CopyMemory((void*)value.GetDataPoint(), len);
    return true;
}

bool CHudpBitStream::Read(char* value, uint16_t len) {
    if (_cur_length + len > _length) {
        return false;
    }
    memcpy((void*)value, _cur_point, len);
    _cur_point += len;
    _cur_length += len;
    return true;
}

bool CHudpBitStream::Read(std::string& value, uint16_t len) {
    if (_cur_length + len > _length) {
        return false;
    }
    value = std::string(_cur_point, len);
    _cur_point += len;
    _cur_length += len;
    return true;
}

bool CHudpBitStream::Read(CHudpBitStream& value, uint16_t len) {
    if (_cur_length + len > _length) {
        return false;
    }
    
    value.Write(_cur_point, len);
    _cur_point += len;
    _cur_length += len;
    return true;
}