#ifndef HEADER_FILTER_SNAPPY_FILTER
#define HEADER_FILTER_SNAPPY_FILTER

#include "IFilter.h"

namespace hudp {
    // snappy filter.
    // compress messages through snappy.
    class CSnappyFilter : public CFilter {
    public:
        CSnappyFilter() {}
        virtual ~CSnappyFilter() {}
        // when send msg filter process
        bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);
        // when recv msg filter process
        bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body);

    private:
        bool SnappyCompress(std::string &in_put, std::string &out_put);
        bool SnappyUncompress(std::string &in_put, std::string &output);
    };
}
#endif