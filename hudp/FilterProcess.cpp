#include "FilterProcess.h"
#include "FilterInterface.h"

using namespace hudp;

bool CFilterProcess::SendProcess(char* data, uint16_t len, uint16_t flag, const std::string& _ip_port) {
    NetMsg* net_msg = nullptr;
    CBitStream* bit_stream = nullptr;
    for (size_t i = __filer_list.size() - 1; i >= 0; i--) {
        if (__filer_list[i]->_handle_phase == PP_UPPER_HANDLE) {
            CHECK_RET(dynamic_cast<CUpperFilterInterface*>(__filer_list[i])->OnSend(data, len, flag, _ip_port, net_msg));
        }
        
        if (__filer_list[i]->_handle_phase == PP_BODY_HANDLE) {
            CHECK_RET(dynamic_cast<CBodyFilterInterface*>(__filer_list[i])->OnSend(data, len));
        }

        if (__filer_list[i]->_handle_phase == PP_PROTO_PARSE) {
            CHECK_RET(dynamic_cast<CProtocolFilterInterface*>(__filer_list[i])->OnSend(net_msg, bit_stream, _ip_port));
        }

        if (__filer_list[i]->_handle_phase == PP_HEAD_HANDLE) {
            CHECK_RET(dynamic_cast<CHeadFilterInterface*>(__filer_list[i])->OnSend(&net_msg->_head));
        }
    }
}

bool CFilterProcess::RecvProcess(CBitStream* bit_stream, const std::string& _ip_port) {
    NetMsg* net_msg = nullptr;
    char* data = nullptr;
    uint16_t len = 0;
    uint16_t flag = 0;
    for (size_t i = __filer_list.size() - 1; i >= 0; i--) {
        if (__filer_list[i]->_handle_phase == PP_PROTO_PARSE) {
            CHECK_RET(dynamic_cast<CProtocolFilterInterface*>(__filer_list[i])->OnRecv(bit_stream, net_msg, _ip_port));
        }

        if (__filer_list[i]->_handle_phase == PP_HEAD_HANDLE) {
            CHECK_RET(dynamic_cast<CHeadFilterInterface*>(__filer_list[i])->OnRecv(&net_msg->_head));
        }

        if (__filer_list[i]->_handle_phase == PP_BODY_HANDLE) {
            CHECK_RET(dynamic_cast<CBodyFilterInterface*>(__filer_list[i])->OnRecv(data, len));
        }

        if (__filer_list[i]->_handle_phase == PP_UPPER_HANDLE) {
            CHECK_RET(dynamic_cast<CUpperFilterInterface*>(__filer_list[i])->OnRecv(data, len, flag, _ip_port, net_msg));
        }
    }
}