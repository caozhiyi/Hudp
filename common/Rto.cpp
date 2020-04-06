#include "Rto.h"
#include "Log.h"
#include "HudpConfig.h"
using namespace hudp;

#define G(x) ((x)>>3)   // 1/8
#define H(x) ((x)>>2)   // 1/4

CRtoImpl::CRtoImpl() : _srtt(0),
                       _mdev(__init_mdev_max),
                       _mdev_max(__init_mdev_max),
                       _cur_rto(__init_cur_rto) {
    
}

CRtoImpl::~CRtoImpl() {
}

void CRtoImpl::SetRttTime(uint64_t rtt) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_srtt == 0) {
        _srtt = (uint32_t)rtt;
        _mdev = (uint32_t)rtt >> 1;

    } else {
        int32_t err = (uint32_t)(rtt - _srtt);
        _srtt = _srtt + G(err);
        _mdev = _mdev + H((int32_t)std::abs(err) - _mdev);
    }
    // get mdev max
    _mdev_max = _mdev > _mdev_max ? _mdev : _mdev_max;
    _cur_rto = _srtt + 4 * _mdev_max;
    base::LOG_DEBUG("cur rto = %d", _cur_rto);
}

uint32_t CRtoImpl::GetRto() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _cur_rto;
}
