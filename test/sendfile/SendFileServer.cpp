#include <map>
#include <string>
#include <fstream>
#include <string.h>
#include <iostream>
#include <string.h> // for strlen

#include "md5.h"
#include "Hudp.h"
#include "Common.h"

using namespace hudp;

struct Content {
    FileHeader _head;
    STATUS     _status;
    Content() {
        _status = hello;
        _file = nullptr;
    }
    ~Content() {
        if (_file) {
            delete _file;
        }
    }
    std::fstream* _file;
};

std::map<HudpHandle, Content> _recv_map;
static const uint16_t __send_flag = HPF_NROMAL_PRI | HTF_RELIABLE;

void ReadFunc(const HudpHandle& handle, const char* msg, uint32_t len, hudp_error_code err) {
    if (_recv_map.count(handle) < 1) {
        _recv_map[handle] = Content();
    }

    Content& content = _recv_map[handle];
    if (content._status == hello) {
        if (len < __header_len) {
            std::cout << "continue recv ... " << std::endl;
            return;
        }

        memcpy((char*)&(content._head), msg, __header_len);

        strncat(content._head._name, ".bk", sizeof(".bk"));

        content._file = new std::fstream(content._head._name, std::ios::out | std::ios::binary);

        std::cout << "get file name   : " << content._head._name << std::endl;
        std::cout << "get file length : " << content._head._length << std::endl;
        std::cout << "get file md5    : " << content._head._md5 << std::endl;
        if (*(content._file)) {
            SendTo(handle, __send_flag,"OK", (uint32_t)strlen("OK"));
            std::cout << "start to recv. " << std::endl;
            content._status = sending;

        } else {
            SendTo(handle, __send_flag, "NO", (uint32_t)strlen("NO"));
            std::cout << "refuse to recv. " << std::endl;
        }

    } else if (content._status == sending) {
        if (len > 0) {
            char buf[__read_len];
            content._head._length -= len;
            content._file->write(buf, len);
            if (content._head._length <= 0) {
                std::cout << "recv end. " << std::endl;
                content._file->sync();
                content._file->close();
                char md5_buf[128] = { 0 };
                Compute_file_md5(content._head._name, md5_buf);
                if (strcmp(md5_buf, content._head._md5) == 0) {
                    SendTo(handle, __send_flag, "OK", (uint32_t)strlen("OK"));
                    std::cout << "recv ok. " << std::endl;

                } else {
                    SendTo(handle, __send_flag, "NO", (uint32_t)strlen("NO"));
                    std::cout << "recv failed. " << std::endl;
                }

            } else {
                std::cout << "recving ... len : " << len << std::endl;
            }
        }
    }
}

void SendCallBack(const HudpHandle& handle, uint32_t upper_id, hudp_error_code err) {
    if (err == hudp::HEC_SUCCESS) {
        std::cout << "send success to :" << handle << " upper_id : " << upper_id << std::endl;
    }
}


int main() {

    hudp::Init();

    hudp::Start("0.0.0.0", 8921, ReadFunc, SendCallBack);

    hudp::Join();
}