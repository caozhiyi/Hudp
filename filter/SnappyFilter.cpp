#include "Log.h"
#include "snappy-c.h"
#include "HudpImpl.h"
#include "SnappyFilter.h"

using namespace hudp;

bool CSnappyFilter::FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    std::string send_msg;
    SnappyCompress(body, send_msg);
    return _next_filter->FilterProcess(handle, flag, send_msg);
}

bool CSnappyFilter::RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
    std::string recv_msg;
    SnappyUncompress(body, recv_msg);
    return _prev_filter->RelieveFilterProcess(handle, flag, recv_msg);
}

bool CSnappyFilter::SnappyCompress(std::string &in_put, std::string &output) {
    char* input_pt = (char*)in_put.data();
    int size = in_put.size();
    size_t out_size = snappy_max_compressed_length(in_put.size());

    output.clear();
    output.resize(out_size);
    if (snappy_compress(input_pt, in_put.size(), (char*)&*output.begin(), &out_size) != SNAPPY_OK) {
        base::LOG_ERROR("compress failed.");
        return false;
    }
    output.resize(out_size);
    return true;
}

bool CSnappyFilter::SnappyUncompress(std::string &in_put, std::string &output) {
    size_t out_size = 0;
    if (snappy_uncompressed_length(in_put.data(), in_put.length(), &out_size) != SNAPPY_OK) {
        base::LOG_ERROR("get out_put size failed");
        return false;
    }
    output.clear();
    output.resize(out_size);
    if (snappy_uncompress((char*)in_put.data(), in_put.length(), (char*)&*output.begin(), &out_size) != SNAPPY_OK) {
        base::LOG_ERROR(" snappy uncompress failed");
        return false;
    }
    return true;
}