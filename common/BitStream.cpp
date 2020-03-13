#include <memory.h> // for memset memcpy

#include "Log.h"
#include "BitStream.h"
#include "CommonFlag.h"
#include "HudpConfig.h"

using namespace hudp;

CBitStream::CBitStream() : _data(nullptr),
                           _cur_point(nullptr),
                           _length(__mtu),
                           _cur_length(0) {
    _data = new char[__mtu];
    memset(_data, 0, __mtu);
    _cur_point = _data;
}

CBitStream::~CBitStream() {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_data) {
        delete _data;
    }
}

uint16_t CBitStream::GetTotalLength() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _length;
}

uint16_t CBitStream::GetCurrentLength() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _cur_length;
}

bool CBitStream::Init(const char* value, uint16_t len) {
    if (!CheckBufferExpend(len)) {
        return false;
    }
    // write value
    std::unique_lock<std::mutex> lock(_mutex);
    memcpy(_data, (void*)value, len);
    _cur_length = len;
    return true;
}

const char* CBitStream::GetDataPoint() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _data;
}

void CBitStream::Clear() {
    std::unique_lock<std::mutex> lock(_mutex);
    memset(_data, 0, _length);
    _cur_point = _data;
    _cur_length = 0;
}

bool CBitStream::CheckBufferExpend(uint16_t len) {
    std::unique_lock<std::mutex> lock(_mutex);
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
        delete []_data;
        _data = temp_data;
    }
    return true;
}

void CBitStream::CopyMemory(void* value, uint16_t len) {
    std::unique_lock<std::mutex> lock(_mutex);
    memcpy(_cur_point, (void*)value, len);
    _cur_point += len;
    _cur_length += len;
}

bool CBitStreamWriter::Write(const char* value, uint16_t len) {
    if (!CheckBufferExpend(len)) {
        return false;
    }
    // write value
    CopyMemory((void*)value, len);
    return true;
}

bool CBitStreamWriter::Write(const std::string& value) {
    uint16_t len = (uint16_t)value.length();
    if (!CheckBufferExpend(len)) {
        return false;
    }
    // write value
    CopyMemory((void*)value.c_str(), len);
    return true;
}

bool CBitStreamWriter::Write(CBitStreamWriter& value) {
    uint16_t len = value.GetCurrentLength();
    if (!CheckBufferExpend(len)) {
        return false;
    }
    // write value
    CopyMemory((void*)value.GetDataPoint(), len);
    return true;
}

bool CBitStreamReader::Read(char* value, uint16_t len) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_cur_point - _data + len > _cur_length) {
        return false;
    }
    memcpy((void*)value, _cur_point, len);
    _cur_point += len;
    return true;
}

bool CBitStreamReader::Read(std::string& value, uint16_t len) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_cur_point - _data + len > _cur_length) {
        return false;
    }
    value = std::string(_cur_point, len);
    _cur_point += len;
    return true;
}

bool CBitStreamReader::Read(CBitStreamReader& value, uint16_t len) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_cur_point - _data + len > _cur_length) {
        return false;
    }
    
    value.Init(_cur_point, len);
    _cur_point += len;
    return true;
}
