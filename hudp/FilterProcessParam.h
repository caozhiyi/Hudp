#ifndef HEADER_HUDP_FILTERPROCESSPARAM
#define HEADER_HUDP_FILTERPROCESSPARAM

#include "CommonType.h"

namespace hudp {

    struct FilterProcessParam {
        enum FilterType {
            FILTER_RECV = 1,
            FILTER_SEND = 2
        };
        FilterType  _filter_type;
        HudpHandle  _handle;
        uint16_t    _flag;
        std::string _body;
        uint32_t    _upper_id;
        FilterProcessParam(FilterType filter_type, const HudpHandle& handle, uint16_t flag, const std::string&& body, uint32_t upper_id = 0) :
            _filter_type(filter_type), _handle(handle), _flag(flag), _body(std::move(body)), _upper_id(upper_id) { }
    };
}

#endif
