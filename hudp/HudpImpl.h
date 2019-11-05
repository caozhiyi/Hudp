#ifndef HEADER_HUDP_HUDPIMPL
#define HEADER_HUDP_HUDPIMPL

#include <vector>
#include "Single.h"
#include "CommonType.h"
#include "HudpFlag.h"

namespace hudp {

    class CNetIO;
    class CThread;
    class CMsg;
    class CMsgFactory;
    class CFilterProcess;
    class CSocketManager;

    class CHudpImpl : public base::CSingle<CHudpImpl> {
    public:
        CHudpImpl();
        ~CHudpImpl();
        // init library
        void Init();
        // start thread and recv
        bool Start(const std::string& ip, uint16_t port, const recv_back& func);

        void Join();

        // send msg
        void SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg);
        void SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint16_t len);

        // destory socket. release resources
        void Close(const HudpHandle& handle);

    public:
        void AfterSendProcess(CMsg* msg);
        void AfterRecvProcess(CMsg* msg);

        // make public flag to private flag of hudp
        //void TranslateFlag(NetMsg* msg);

    private:
        std::vector<std::shared_ptr<CThread>>      _thread_vec;
        std::shared_ptr<CNetIO>                    _net_io;
        std::shared_ptr<CMsgFactory>               _msg_factory;
        std::shared_ptr<CFilterProcess>            _filter_process;
        std::shared_ptr<CSocketManager>            _socket_mananger;
        recv_back                                  _recv_call_back;
        uint64_t                                   _listen_socket;
    };

}

#endif
