#include "Rto.h"
#include "Log.h"
using namespace hudp;

#define G(x) (x)>>3   // 1/8
#define H(x) (x)>>2   // 1/4

static const uint32_t __init_mdev_max = 15;     // 15ms. so min rto = 60ms
static const uint32_t __init_cur_rto  = 1000;   // 1000ms.

CRto::CRto() : _srtt(0), _cur_rto(__init_cur_rto), _mdev(__init_mdev_max), _mdev_max(__init_mdev_max) {
    
}

CRto::~CRto() {
    std::unique_lock<std::mutex> lock(_mutex);
    _id_time.clear();
}

void CRto::SetIdTime(uint16_t id, uint64_t time) {
    std::unique_lock<std::mutex> lock(_mutex);
    _id_time[id] = time;
}

void CRto::RemoveIdTime(uint16_t id) {
    std::unique_lock<std::mutex> lock(_mutex);
    _id_time.erase(id);
}

void CRto::SetAckTime(uint16_t id, uint64_t time) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _id_time.find(id);
    if (iter == _id_time.end()) {
        return;
    }
    
    // repeat ack
    if (time <= iter->second) {
        return;
    }

    uint32_t m =(uint32_t)(time - iter->second);
    if (_srtt == 0) {
        _srtt = m;
        _mdev = m >> 1;

    } else {
        int32_t err = m - _srtt;
        _srtt = _srtt + G(err);
        _mdev = _mdev + H((int32_t)std::abs(err) - _mdev);
    }
    // get mdev max
    _mdev_max = _mdev > _mdev_max ? _mdev : _mdev_max;
    _cur_rto = _srtt + 4 * _mdev_max;
    base::LOG_DEBUG("cur rto = %d", _cur_rto);
}

uint32_t CRto::GetRto() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _cur_rto;
}