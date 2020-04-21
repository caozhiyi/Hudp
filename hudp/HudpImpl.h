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
        hudp_error_code Init();
        // start thread and recv
        hudp_error_code Start(const std::string& ip, uint16_t port, const recv_back& recv_func, 
                              const can_write_back& can_write_func);

        hudp_error_code Join();

        // send msg
        hudp_error_code SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg);

        // check socket can send?
        hudp_error_code CheckCanSend(const HudpHandle& handle);

        // connect to remote
        hudp_error_code ConnectTo(const std::string& ip, uint16_t port);

        // destroy socket. release resources
        hudp_error_code Close(const HudpHandle& handle);

        // recv msg
        void RecvMsgFromNet(const HudpHandle& handle, const std::string& msg);

    public:
        // notify supper recv a message.
        void RecvMessageToUpper(const HudpHandle& handle, std::string& );
        // send message to net
        void SendMessageToNet(std::shared_ptr<CMsg> msg);
        // get a default priority queue instance
        CPriorityQueue* CreatePriorityQueue();
        // release socket
        void ReleaseSocket(const HudpHandle& handle);
        // send msg to send filter, final will send msg to socket
        bool SendMsgToFilter(const HudpHandle& handle, uint16_t flag, std::string& msg);
        // recv msg to recv filter, filal will recv msg to upper
        bool RecvMsgToFilter(const HudpHandle& handle, uint16_t flag, std::string& msg);

    private:
        std::shared_ptr<CNetIO>                    _net_io;
        std::shared_ptr<CFilterProcess>            _filter_process;
        std::shared_ptr<CSocketManager>            _socket_mananger;

        recv_back                                  _recv_call_back;
        can_write_back                             _can_write_call_back;

        uint64_t                                   _listen_socket;

        std::shared_ptr<CProcessThread>            _process_thread;
        std::shared_ptr<CRecvThread>               _recv_thread;
    };

}

#endif