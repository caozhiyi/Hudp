//#include "SendWndSolt.h"
//#include "SendWnd.h"
//#include "../Test.h"
//
//using namespace hudp;
//
//CSendWnd __send_wnd(2);
//static bool __run = true;
//
//class CWndMsg : public CSendWndSolt {
//public:
//    CWndMsg(uint16_t id) : _id(id) {}
//    virtual ~CWndMsg() {}
//
//    void ToSend() {
//        std::cout << "to send msg id :" << _id << std::endl;
//    }
//
//    void AckDone() {
//        std::cout << "ack done msg id:" << _id << std::endl;
//    }
//
//    uint16_t Id() {
//        return _id;
//    }
//private:
//    uint16_t _id;
//};
//
//void UtestCSendWnd() {
//
//    CWndMsg msg1(1);
//    CWndMsg msg2(2);
//    CWndMsg msg3(3);
//    CWndMsg msg4(4);
//    CWndMsg msg5(5);
//    
//    __send_wnd.PushBack(msg1.Id(), &msg1);
//    __send_wnd.PushBack(msg2.Id(), &msg2);
//    __send_wnd.PushBack(msg3.Id(), &msg3);
//
//    __send_wnd.AcceptAck(msg2.Id());
//
//    __send_wnd.SetIndexResend(msg1.Id());
//
//    __send_wnd.PushBack(msg4.Id(), &msg4);
//    __send_wnd.PushBack(msg5.Id(), &msg5);
//
//    __send_wnd.AcceptAck(msg1.Id());
//    __send_wnd.ChangeSendWndSize(3);
//
//    __send_wnd.AcceptAck(msg3.Id());
//    __send_wnd.AcceptAck(msg5.Id());
//    __send_wnd.AcceptAck(msg4.Id());
//
//
//    __send_wnd.PushBack(msg1.Id(), &msg1);
//    __send_wnd.PushBack(msg2.Id(), &msg2);
//
//    __send_wnd.AcceptAck(msg1.Id(), 2);
//
//    __send_wnd.PushBack(msg3.Id(), &msg3);
//    __send_wnd.AcceptAck(msg3.Id());
//    
//}