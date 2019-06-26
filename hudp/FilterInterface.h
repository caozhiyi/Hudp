#ifndef HEADER_HUDP_FILTERINTERFACE
#define HEADER_HUDP_FILTERINTERFACE

#include <string>
#include <vector>

#include "CommonFlag.h"
#include "NetMsg.h"

namespace hudp {

    class CFilterInterface;
    static std::vector<CFilterInterface*> __filer_list;
    static process_phase __cur_handle_phase;

    class CBitStream;
    class CFilterInterface {
    public:
        CFilterInterface(process_phase phase) : _handle_phase(phase) {}
        virtual ~CFilterInterface() {}
        
        // attach to chain of responsibility
        bool Attach();
        // remove from chain of responsibility
        bool Relieve();
        // get phase
        const process_phase& GetPhase();

    private:
        friend class CFilterProcess;

        process_phase _handle_phase;
    };

    // protocol resolution
    class CProtocolFilterInterface : public CFilterInterface {
    public:
        CProtocolFilterInterface() : CFilterInterface(PP_PROTO_PARSE) {}
        virtual ~CProtocolFilterInterface() {}

        virtual bool OnSend(NetMsg* msg, CBitStream* bit_stream, const std::string& _ip_port) = 0;
        virtual bool OnRecv(CBitStream* bit_stream, NetMsg* msg, const std::string& _ip_port) = 0;
    };

    // head handle
    class CHeadFilterInterface : public CFilterInterface {
    public:
        CHeadFilterInterface() : CFilterInterface(PP_HEAD_HANDLE) {}
        virtual ~CHeadFilterInterface() {}

        virtual bool OnSend(Head* msg) = 0;
        virtual bool OnRecv(Head* msg) = 0;
    };

    // body handle
    class CBodyFilterInterface : public CFilterInterface {
    public:
        CBodyFilterInterface() : CFilterInterface(PP_BODY_HANDLE) {}
        virtual ~CBodyFilterInterface() {}

        virtual bool OnSend(char* msg, uint16_t len) = 0;
        virtual bool OnRecv(char* msg, uint16_t& len) = 0;
    };
    
    // upper program handle
    class CUpperFilterInterface : public CFilterInterface {
    public:
        CUpperFilterInterface() : CFilterInterface(PP_UPPER_HANDLE) {}
        virtual ~CUpperFilterInterface() {}

        virtual bool OnSend(char* data, uint16_t len, uint16_t flag, const std::string& _ip_port, NetMsg* msg) = 0;
        virtual bool OnRecv(char* data, uint16_t& len, uint16_t& flag, const std::string& _ip_port, NetMsg* msg) = 0;
    };
}

#endif