#ifndef HEADER_HUDP_HUDPIMPL
#define HEADER_HUDP_HUDPIMPL

#include <memory>
#include "Single.h"
#include "CommonType.h"
#include "HudpFlag.h"

namespace hudp {

    class CNetIO;
    class CMsg;
    class CMsgFactory;
    class CFilterProcess;
    class CSocketManager;
    class CProcessThread;
    class CRecvThread;

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
        void SendTo(const HudpHandle& Hhandle, uint16_t flag, const char* msg, uint16_t len);

        // recv msg
        void RecvMsg(const HudpHandle& handle, const std::string& msg);

        // destory socket. release resources
        void Close(const HudpHandle& handle);

    public:
        // notify aupper recv a message.
        void RecvMessageToUpper(const HudpHandle& handle, CMsg* msg);
        // send message to net
        void SendMessageToNet(CMsg* msg);
        // release send message
        void ReleaseMessage(CMsg* msg);

    public:
        void AfterSendProcess(CMsg* msg);
        void AfterRecvProcess(CMsg* msg);

    private:
        std::shared_ptr<CNetIO>                    _net_io;
        std::shared_ptr<CMsgFactory>               _msg_factory;
        std::shared_ptr<CFilterProcess>            _filter_process;
        std::shared_ptr<CSocketManager>            _socket_mananger;
        recv_back                                  _recv_call_back;
        uint64_t                                   _listen_socket;

        std::shared_ptr<CProcessThread>            _process_thread;
        std::shared_ptr<CRecvThread>               _recv_thread;
    };

}

#endif
