#ifndef HEADER_HUDP_FILTERPROCESS
#define HEADER_HUDP_FILTERPROCESS

#include <string>

namespace hudp {

    class CHudpBitStream;
    class CFilterProcess {
    public:
        CFilterProcess() {}
        virtual ~CFilterProcess() {}
        
        // process chain of responsibility
        bool SendProcess(char* data, uint16_t len, uint16_t flag, const std::string& _ip_port);
        bool RecvProcess(CHudpBitStream* bit_stream, const std::string& _ip_port); 
    };
}

#endif