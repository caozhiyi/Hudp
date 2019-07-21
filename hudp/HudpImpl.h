#ifndef HEADER_NET_HUDPIMPL
#define HEADER_NET_HUDPIMPL

#include <functional>
#include "Single.h"
#include "CommonType.h"
#include "CommonFlag.h"
#include "ProcessThread.h"
#include "RecvThread.h"
#include "SendThread.h"
#include "UpperThread.h"
#include "SocketManager.h"

namespace hudp {

    class NetMsg;
    class CHudpImpl : public base::CSingle<CHudpImpl> {
    public:
        CHudpImpl();
        ~CHudpImpl();
        // init library
        void Init(bool log = false);
        // start thread and recv
        bool Start(uint16_t port, const recv_back& func);

        void Join();

        // send msg
        void SendTo(const HudpHandle& handlle, uint16_t flag, const std::string& msg);
        void SendTo(const HudpHandle& handlle, uint16_t flag, const char* msg, uint16_t len);

        // destory socket. release resources
        void Destroy(const HudpHandle& handlle);

    public:
        // send msg to send thread
        void SendMsgToNet(NetMsg* msg);
        // send msg to upper thread;
        void SendMsgToUpper(NetMsg* msg);
        // send msg to recv process thread
        void SendMsgToRecvProcessThread(NetMsg* msg);
        // send msg to send process thread
        void SendMsgToSendProcessThread(NetMsg* msg);

        // make public flag to private flag of hudp
        void TranslateFlag(NetMsg* msg);

    private:
        CRecvProcessThread      _recv_process_thread;
        CSendProcessThread      _send_process_thread;
        CRecvThread             _recv_thread;
        CSendThread             _send_thread;
        CUpperThread            _upper_thread;
    };

}

#endif