#ifndef HEADER_INTERFACE_FILTER
#define HEADER_INTERFACE_FILTER

#include <string> // for string

namespace hudp {
    // msg body filter.
    // If you want to add a inclusion process, can inherit this interface.
    class CFilter
    {
    public:
        CFilter() {}
        virtual ~CFilter() {}
        // when send msg filter process
        virtual void FilterProcess(std::string& body) = 0;
        // when recv msg filter process
        virtual void RelieveFilterProcess(std::string& body) = 0;
    };
}
#endif