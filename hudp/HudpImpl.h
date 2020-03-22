#ifndef HEADER_HUDP_HUDPIMPL
#define HEADER_HUDP_HUDPIMPL

#include <memory>

#include "Single.h"
#include "HudpFlag.h"
#include "CommonType.h"

namespace hudp {
    
    class CMsg;
    class CNetIO;
    class CRecvThread;
    class CFilterProcess;
    class CSocketManager;
    class CProcessThread;
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
        bool SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg);
        bool SendTo(const HudpHandle& Hhandle, uint16_t flag, const char* msg, uint16_t len);

        // recv msg
        void RecvMsgFromNet(const HudpHandle& handle, const std::string& msg);

        // destroy socket. release resources
        void Close(const HudpHandle& handle);

    public:
        // notify supper recv a message.
        void RecvMessageToUpper(const HudpHandle& handle, std::shared_ptr<CMsg> msg);
        // send message to net
        void SendMessageToNet(std::shared_ptr<CMsg> msg);
        // get a default priority queue instance
        CPriorityQueue* CreatePriorityQueue();
        // release socket
        void ReleaseSocket(const HudpHandle& handle);

    public:
        void AfterSendFilter(std::shared_ptr<CMsg> msg);
        void AfterRecvFilter(std::shared_ptr<CMsg> msg);

    private:
        std::shared_ptr<CNetIO>                    _net_io;
        std::shared_ptr<CFilterProcess>            _filter_process;
        std::shared_ptr<CSocketManager>            _socket_mananger;
        recv_back                                  _recv_call_back;
        uint64_t                                   _listen_socket;

        std::shared_ptr<CProcessThread>            _process_thread;
        std::shared_ptr<CRecvThread>               _recv_thread;
    };

}

#endif