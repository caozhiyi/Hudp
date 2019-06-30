#include "Timer.h"
#include "TimerSolt.h"
#include "CommonFlag.h"
#include "TimeTool.h"
#include "../Test.h"

using namespace hudp;

base::CTimeTool __tt;

class CTestTimer : public CTimerSolt {
    public:
        CTestTimer(uint16_t id) : _id(id) {}
        virtual ~CTestTimer() {}
        virtual void OnTimer() {
            __tt.Now();
            std::cout << "timer call back : " << _id << " time : " << __tt.GetMsec() << std::endl;
        }
        uint16_t Id() {
            return _id;
        }
private:
    uint16_t _id;
};

void UtestTimer() {

    CTimer timer;
    timer.Start();

    CTestTimer t1(1);
    CTestTimer t2(2);
    CTestTimer t3(3);
    CTestTimer t4(4);

    t1.Attach(&timer, 21);
    t2.Attach(&timer, 22);
    t3.Attach(&timer, 23);
    t4.Attach(&timer, 24);

    CTestTimer t5(5);
    CTestTimer t6(6);
    CTestTimer t7(7);
    CTestTimer t8(8);

    t5.Attach(&timer, 101);
    t6.Attach(&timer, 102);
    t7.Attach(&timer, 103);
    t8.Attach(&timer, 104);

    CTestTimer t9(9);
    CTestTimer t10(10);
    CTestTimer t11(11);
    CTestTimer t12(12);

    t9.Attach(&timer, 201);
    t10.Attach(&timer, 202);
    t11.Attach(&timer, 203);
    t12.Attach(&timer, 204);

    CTestTimer t13(13);
    CTestTimer t14(14);
    CTestTimer t15(15);
    CTestTimer t16(16);

    t13.Attach(&timer, 401);
    t14.Attach(&timer, 402);
    t15.Attach(&timer, 403);
    t16.Attach(&timer, 404);

    CTestTimer t17(17);
    CTestTimer t18(18);
    CTestTimer t19(19);
    CTestTimer t20(20);

    t17.Attach(&timer, 801);
    t18.Attach(&timer, 802);
    t19.Attach(&timer, 803);
    t20.Attach(&timer, 804);

    CTestTimer t21(21);
    CTestTimer t22(22);
    CTestTimer t23(23);
    CTestTimer t24(24);


    t21.Attach(&timer, 2001);
    t22.Attach(&timer, 2002);
    t23.Attach(&timer, 2003);
    t24.Attach(&timer, 2004);

    base::CRunnable::Sleep(5000);
    
}