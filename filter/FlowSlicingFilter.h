#ifndef HEADER_FILTER_FLOW_SLICING
#define HEADER_FILTER_FLOW_SLICING

#include <map>
#include <atomic>
#include "IFilter.h"

namespace hudp {
    // slice head info
    struct SliceHead {
        uint16_t _count;    // msg total number of slice
        uint16_t _index;    // current slice index of total slice
        uint16_t _flag;     // identify different msgs
    };

    // slice bag, hold message value
    struct SliceBag {
        SliceHead   _head;
        std::string _body;
    };

    // slice bag, reference message values only
    struct SliceBagRef {
        SliceHead   _head;
        char*       _data;
        uint32_t    _data_len;
    };

    // the flow slicing filter
    // divide the stream into packets smaller than mtu.
    class CFlowSlicingFilter : public CFilter {
    public:
        CFlowSlicingFilter() : _cur_flag(0) {}
        virtual ~CFlowSlicingFilter() {}
        // when send msg filter process
        bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id);
        // when recv msg filter process
        bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
    private:
        std::string SliceBagToString(SliceBag& bag);
        std::string SliceBagRefToString(SliceBagRef& bag);
        SliceBag StringToSliceBag(std::string& msg);
        
        std::atomic_uint16_t _cur_flag;
        std::map<uint16_t, std::vector<SliceBag>> _slice_map;
    };
}
#endif