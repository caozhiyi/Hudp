//#include "UpperThread.h"
//#include "BitStreamPool.h"
//#include "NetMsgPool.h"
//#include "OsNet.h"
//#include "Log.h"
//using namespace hudp;
//
//CUpperThread::CUpperThread() {
//
//}
//
//CUpperThread::~CUpperThread() {
//    Stop();
//    Push(nullptr);
//    Join();
//}
//
//void CUpperThread::Start(const recv_back& func) {
//    _call_back = func;
//    CRunnable::Start();
//}
//
//void CUpperThread::Run() {
//    while (!_stop) {
//        auto pt = static_cast<NetMsg*>(_Pop());
//        if (pt) {
//            _call_back(pt->_ip_port, pt->_body, pt->_head._body_len);
//            CBitStreamPool::Instance().FreeBitStream(pt->_bit_stream);
//            CNetMsgPool::Instance().FreeMsg(pt, true);
//
//        } else {
//            base::LOG_WARN("upper thread get a null msg.");
//        }
//    }
//}
