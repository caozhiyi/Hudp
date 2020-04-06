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

void ReadFunc(const HudpHandle& handle, const char* msg, uint32_t len) {
    auto iter = _recv_map.find(handle);
    if (iter == _recv_map.end()) {
        _recv_map[handle] = Content();
    }

    if (iter->second._status == hello) {
        if (len < __header_len) {
            std::cout << "continue recv ... " << std::endl;
            return;
        }

        memcpy((char*)&(iter->second._head), msg, __header_len);

        strncat(iter->second._head._name, ".bk", sizeof(".bk"));

        iter->second._file = new std::fstream(iter->second._head._name, std::ios::out | std::ios::binary);

        std::cout << "get file name   : " << iter->second._head._name << std::endl;
        std::cout << "get file length : " << iter->second._head._length << std::endl;
        std::cout << "get file md5    : " << iter->second._head._md5 << std::endl;
        if (*(iter->second._file)) {
            SendTo(handle, __send_flag,"OK", strlen("OK"));
            std::cout << "start to recv. " << std::endl;
            iter->second._status = sending;

        } else {
            SendTo(handle, __send_flag, "NO", strlen("NO"));
            std::cout << "refuse to recv. " << std::endl;
        }

    } else if (iter->second._status == sending) {
        while (len > 0) {
            char buf[__read_len];
            iter->second._head._length -= len;
            iter->second._file->write(buf, len);
            if (iter->second._head._length <= 0) {
                std::cout << "recv end. " << std::endl;
                iter->second._file->sync();
                iter->second._file->close();
                char md5_buf[128] = { 0 };
                Compute_file_md5(iter->second._head._name, md5_buf);
                if (strcmp(md5_buf, iter->second._head._md5) == 0) {
                    SendTo(handle, __send_flag, "OK", strlen("OK"));
                    std::cout << "recv ok. " << std::endl;

                } else {
                    SendTo(handle, __send_flag, "NO", strlen("NO"));
                    std::cout << "recv failed. " << std::endl;
                }
            }
        }
    }
}

int main() {

    hudp::Init();

    hudp::Start("0.0.0.0", 8921, ReadFunc);

    hudp::Join();
}