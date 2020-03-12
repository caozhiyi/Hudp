#ifndef HEADER_HUDP_HUDPIMPL
#define HEADER_HUDP_HUDPIMPL

#include <memory>

#include "Single.h"
#include "HudpFlag.h"
#include "CommonType.h"

namespace hudp {

    class CNetIO;
    class CMsg;
    class CMsgFactory;
    class CFilterProcess;
    class CSocketManager;
    class CProcessThread;
    class CRecvThread;
    class CPriorityQueue;

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
        void RecvMsgFromNet(const HudpHandle& handle, const std::string& msg);

        // destroy socket. release resources
        void Close(const HudpHandle& handle);

    public:
        // notify supper recv a message.
        void RecvMessageToUpper(const HudpHandle& handle, CMsg* msg);
        // send message to net
        void SendMessageToNet(CMsg* msg);
        // release send message
        void ReleaseMessage(CMsg* msg);
        // create a msg
        CMsg* CreateMessage();
        // get a default priority queue instance
        CPriorityQueue* CreatePriorityQueue();

    public:
        void AfterSendFilter(CMsg* msg);
        void AfterRecvFilter(CMsg* msg);

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