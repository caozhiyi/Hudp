//#include "PriorityQueue.h"
//#include "NetMsg.h"
//#include "CommonFlag.h"
//#include "../Test.h"
//#include "Runnable.h"
//
//#include <thread>
//#include <unordered_map>
//
//using namespace hudp;
//
//CPriorityQueue __queue;
//
//NetMsg LowMsg;
//NetMsg NormalMsg;
//NetMsg HighMsg;
//NetMsg HighestMsg;
//
//std::unordered_map<uint16_t, std::string> __map;
//static bool __run = true;
//
//void Init() {
//    LowMsg._head._flag     |= HPF_LOW_PRI;
//    NormalMsg._head._flag  |= HPF_NROMAL_PRI;
//    HighMsg._head._flag    |= HPF_HIGH_PRI;
//    HighestMsg._head._flag |= HPF_HIGHEST_PRI;
//
//    __map[HPF_LOW_PRI] = "HPF_LOW_PRI";
//    __map[HPF_NROMAL_PRI] = "HPF_NROMAL_PRI";
//    __map[HPF_HIGH_PRI] = "HPF_HIGH_PRI";
//    __map[HPF_HIGHEST_PRI] = "HPF_HIGHEST_PRI";
//}
//
//void PushMsg(NetMsg* msg) {
//    while (__run) {
//        __queue.Push(msg);
//        base::CRunnable::Sleep(500);
//    }
//}
//
//void UtestCPriorityQueue() {
//    
//    Init();
//
//    std::thread thread1(PushMsg, &LowMsg);
//    std::thread thread2(PushMsg, &NormalMsg);
//    std::thread thread3(PushMsg, &HighMsg);
//    std::thread thread4(PushMsg, &HighestMsg);
//
//    base::CRunnable::Sleep(5000);
//
//    uint16_t times = 100;
//    NetMsg* temp_msg;
//    while (times--) {
//        temp_msg = __queue.Pop();
//        std::cout << times << " : msg pri : " << __map[temp_msg->_head._flag] << std::endl;
//    }
//
//    __run = false;
//
//    thread1.join();
//    thread2.join();
//    thread3.join();
//    thread4.join();
//}