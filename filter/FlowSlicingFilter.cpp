#include <algorithm>
#include "HudpConfig.h"
#include "FlowSlicingFilter.h"

using namespace hudp;

static const uint16_t __slice_header_size = sizeof(SliceHead);
static const uint16_t __slice_limit = __msg_body_size - __slice_header_size;

static bool SliceBagComparer(const SliceBag& bag1, const SliceBag& bag2) {
    return bag1._head._index < bag2._head._index;
}

bool CFlowSlicingFilter::FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    _cur_flag.fetch_add(1);

    uint32_t total_size = (uint32_t)body.length();
    uint32_t slice_num = total_size / __slice_limit;
    if (total_size % __slice_limit > 0) {
        slice_num++;
    }
    // slicing msg to send
    char* start = (char*)body.data();
    uint32_t cur_index = 0;
    SliceBagRef bag;
    bag._head._count = slice_num;
    bag._head._flag  = _cur_flag.load();
    for (uint32_t i = 0; i < slice_num; i++) {
        bag._head._index = i;
        if (total_size - cur_index < __slice_limit) {
            bag._data = start + cur_index;
            bag._data_len =  total_size - cur_index;
            cur_index += total_size - cur_index;

        } else {
            bag._data = start + cur_index;
            bag._data_len =  __slice_limit;
            cur_index += __slice_limit;
        }
        // send to next filter
        std::string send_slice =  SliceBagRefToString(bag);
        if (!_next_filter->FilterProcess(handle, flag, send_slice)) {
            return false;
        }
    }
    return true;
}

bool CFlowSlicingFilter::RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    SliceBag bag = StringToSliceBag(body);
    _slice_map[bag._head._flag].push_back(bag);
    // recv all slice
    if (bag._head._count == (uint16_t)(_slice_map[bag._head._flag].size())) {
        std::string recv_msg;
        std::sort(_slice_map[bag._head._flag].begin(), _slice_map[bag._head._flag].end(), SliceBagComparer);
        for (auto iter = _slice_map[bag._head._flag].begin(); iter != _slice_map[bag._head._flag].end(); iter++) {
            recv_msg.append(iter->_body);
        }
        _slice_map.erase(bag._head._flag);
        // send to prev filter
        return _prev_filter->RelieveFilterProcess(handle, flag, recv_msg);
    }
    return true;
}

std::string CFlowSlicingFilter::SliceBagToString(SliceBag& bag) {
    char header_buf[__slice_header_size] = { 0 };
    memcpy(header_buf, &bag._head, __slice_header_size);
    
    std::string ret;
    ret.resize(__slice_header_size + bag._body.length());
    ret.append(header_buf, __slice_header_size);
    ret.append(bag._body);
    return ret;
}

std::string CFlowSlicingFilter::SliceBagRefToString(SliceBagRef& bag) {
    char header_buf[__slice_header_size] = { 0 };
    memcpy(header_buf, &bag._head, __slice_header_size);
    
    std::string ret;
    ret.append(header_buf, __slice_header_size);
    ret.append(bag._data, bag._data_len);
    return ret;
}

SliceBag CFlowSlicingFilter::StringToSliceBag(std::string& msg) {
    SliceBag bag;

    char* start = (char*)msg.data();
    memcpy(&bag._head, start, __slice_header_size);

    bag._body = std::string(start + __slice_header_size, msg.length() - __slice_header_size);
    return bag;
}