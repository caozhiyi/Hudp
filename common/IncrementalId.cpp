#include "HudpConfig.h"
#include "IncrementalId.h"

using namespace hudp;

static const uint16_t __max_id = (uint16_t)0xFFFF;

CIncrementalId::CIncrementalId() : _cur_id(0){

}

CIncrementalId::~CIncrementalId() {

}

uint16_t CIncrementalId::GetNextId() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cur_id++;
    return _cur_id;
}

bool CIncrementalId::IsNextId(uint16_t prev, uint16_t next) {
    return static_cast<uint16_t>(prev + 1) == next;
}

bool CIncrementalId::IsSmallerThan(const uint16_t& prev, const uint16_t& next) {
    return IsBiggerThan(next, prev);
}

bool CIncrementalId::IsBiggerThan(const uint16_t& prev, const uint16_t& next) {
    if (prev <= __max_compare_num && next > (__max_id - 100)) {
        return true;

    } else if (next <= __max_compare_num && prev > (__max_id - 100)) {
        return false;        

    } else {
        if (prev > next) {
            return true;
        }
    }

    return false;
}

