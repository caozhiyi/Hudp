#include "ProcessThread.h"
#include "FilterProcess.h"
#include "HudpImpl.h"
using namespace hudp;

CRecvProcessThread::CRecvProcessThread() {

}

CRecvProcessThread::~CRecvProcessThread() {
    Stop();
    Push(nullptr);
    Join();
}

void CRecvProcessThread::Run() {
    while (!_stop) {
        auto pt = _Pop();
        if (pt) {
            CFilterProcess::Instance().RecvProcess((NetMsg*)pt);
        }
    }
}

CSendProcessThread::CSendProcessThread() {

}

CSendProcessThread::~CSendProcessThread() {
    Stop();
    Push(nullptr);
    Join();
}

void CSendProcessThread::Run() {
    while (!_stop) {
        auto pt = CSocketManager::Instance().GetMsg();
        if (pt) {
            CFilterProcess::Instance().SendProcess((NetMsg*)pt);
        }
    }
}