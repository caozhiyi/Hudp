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
                              const send_back& write_func);

        hudp_error_code Join();

        // send msg
        hudp_error_code SendTo(const HudpHandle& handle, uint16_t flag, const std::string& msg);

        // recv msg
        void RecvMsgFromNet(const HudpHandle& handle, const std::string& msg);

        // destroy socket. release resources
        hudp_error_code Close(const HudpHandle& handle);

    public:
        // notify supper about.
        void RecvMessageToUpper(const HudpHandle& handle, std::string& msg, hudp_error_code err = HEC_SUCCESS);
        void NewConnectToUpper(const HudpHandle& handle, hudp_error_code err);
        void ResendBackToUpper(const HudpHandle& handle, const char* msg, uint32_t len, bool& continue_send);
        void SendBackToUpper(const HudpHandle& handle, const char* msg, uint32_t len, hudp_error_code err);

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
        // call back about
        void SetConnectCallBack(const connect_back& conn_func);
        void SetResendCallBack(const resend_back& resend_func);
        // check socket can send?
        hudp_error_code CheckCanSend(const HudpHandle& handle);

    private:
        std::shared_ptr<CNetIO>                    _net_io;
        std::shared_ptr<CFilterProcess>            _filter_process;
        std::shared_ptr<CSocketManager>            _socket_mananger;
        uint64_t                                   _listen_socket;

        // call back function
        recv_back                                  _recv_call_back;
        send_back                                  _send_call_back;
        resend_back                                _resend_call_back;
        connect_back                               _connect_call_back;

        std::shared_ptr<CProcessThread>            _process_thread;
        std::shared_ptr<CRecvThread>               _recv_thread;
    };

}

#endif