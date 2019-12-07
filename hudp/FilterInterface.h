//#ifndef HEADER_HUDP_FILTERINTERFACE
//#define HEADER_HUDP_FILTERINTERFACE
//
//#include <string>
//#include <vector>
//#include "CommonFlag.h"
//
//namespace hudp {
//
//    class NetMsg;
//
//    class CFilterInterface {
//    public:
//        CFilterInterface(uint8_t phase);
//        virtual ~CFilterInterface();
//        
//        // attach to chain of responsibility
//        bool Attach();
//        // remove from chain of responsibility
//        bool Relieve();
//        // get phase
//        const uint8_t& GetPhase();
//
//        // set msg to next phase
//        void NextPhase(NetMsg* msg);
//
//        // process HudpHandle
//        virtual bool OnSend(NetMsg* msg) = 0;
//        virtual bool OnRecv(NetMsg* msg) = 0;
//
//    private:
//        uint8_t _HudpHandle_phase;
//    };
//}
//
//#endif
