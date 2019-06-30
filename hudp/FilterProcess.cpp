#include "FilterProcess.h"
#include "FilterInterface.h"

using namespace hudp;

void CFilterProcess::SendProcess(NetMsg* msg) {
    NetMsg* net_msg = nullptr;
    CBitStream* bit_stream = nullptr;
    for (size_t i = __filer_list.size() - 1; i >= 0; i--) {
        if (__filer_list[i]->_handle_phase == PP_UPPER_HANDLE) {
            dynamic_cast<CUpperFilterInterface*>(__filer_list[i])->OnSend(msg);
        }
        
        if (__filer_list[i]->_handle_phase == PP_BODY_HANDLE) {
            dynamic_cast<CBodyFilterInterface*>(__filer_list[i])->OnSend(msg);
        }

        if (__filer_list[i]->_handle_phase == PP_PROTO_PARSE) {
            dynamic_cast<CProtocolFilterInterface*>(__filer_list[i])->OnSend(msg);
        }

        if (__filer_list[i]->_handle_phase == PP_HEAD_HANDLE) {
            dynamic_cast<CHeadFilterInterface*>(__filer_list[i])->OnSend(msg);
        }
    }
}

void CFilterProcess::RecvProcess(NetMsg* msg) {
    NetMsg* net_msg = nullptr;
    char* data = nullptr;
    uint16_t len = 0;
    uint16_t flag = 0;
    for (size_t i = __filer_list.size() - 1; i >= 0; i--) {
        if (__filer_list[i]->_handle_phase == PP_PROTO_PARSE) {
            dynamic_cast<CProtocolFilterInterface*>(__filer_list[i])->OnRecv(msg);
        }

        if (__filer_list[i]->_handle_phase == PP_HEAD_HANDLE) {
            dynamic_cast<CHeadFilterInterface*>(__filer_list[i])->OnRecv(msg);
        }

        if (__filer_list[i]->_handle_phase == PP_BODY_HANDLE) {
            dynamic_cast<CBodyFilterInterface*>(__filer_list[i])->OnRecv(msg);
        }

        if (__filer_list[i]->_handle_phase == PP_UPPER_HANDLE) {
            dynamic_cast<CUpperFilterInterface*>(__filer_list[i])->OnRecv(msg);
        }
    }
}