#ifndef HEADER_HUDP_HUDPCONFIG
#define HEADER_HUDP_HUDPCONFIG

#include "Log.h"
#include "CommonType.h"

namespace hudp {

    // msg mtu size, hudp may print out error when msg body's size is bigger than it.
    static const uint16_t __mtu = 400;

    // bitstream will return false when length of bag is bigger than __mtu.
    static const bool     __must_less_mtu = true;

    // max length of bitstream bag.
    static const uint16_t __max_length = (uint16_t)0xFFFF;

    // bitstream pool expand size every times.
    static const uint16_t _stream_pool_expand_size = 100;

    // if id trans boundary, still think bigger in 0 ~ __max_compare_num.
    static const uint16_t __max_compare_num = 100;

    // msg of body'size, must littler than mtu.
    static const uint16_t __msg_body_size = __mtu;

    // priority queue send __pri_surplus high-level packages and one low-level package when busy.
    static const uint8_t  __pri_surplus = 2;

    // reliable msg receiver cache msg size to check repeat.
    static const uint16_t __msx_cache_msg_num = 256;

    // timer is empty. wait timer
    static const uint16_t __timer_empty_wait = 60000;

    // recvfrom receive buffer size
    static const uint16_t __recv_buf_size = 2048;

    // send window size when start
    static const uint16_t __init_send_wnd_size = 6;

    // pend ack timer wait time.
    static const uint16_t __pend_ack_send = 100;    // 100ms

    // max rto time after retreat
    static const uint16_t __max_rto_time = 12000;  // max rto 120s

    // message resend timer
    static const uint16_t __resend_time = 150;      // 150ms

    // if out of order ack is more than it, resend quickly
    static const uint16_t __quick_resend_limit = 3;

    // 15ms. so min rto = 60ms
    static const uint32_t __init_mdev_max = 15;
    // 1000ms.
    static const uint32_t __init_cur_rto = 1000;
    // msg with send time stamp, to calculation rtt time
    static const bool     __msg_with_time = true;

    // when close socket, msg in cache should send complete?
    static const bool     __send_all_msg_when_close = true;
    // msl time
    static const uint32_t __2_msl_time = 30000;   // 30s

    // message pool size if bigger than it, will reduce half of queue size
    static const uint16_t __msg_pool_reduce_limit_size = 50;

    // if discard msg count is bigger than it, send resrt to remote
    static const uint16_t __msg_discard_limit = 3;

    // default pacing rate, if connect can't calc pacing will use it
    static const uint32_t __defaule_pacing_rate = 0xFFFF;

    // use pacing and flow queue control send flow?
    static const bool     __use_fq_and_pacing = true;

    // about log setting
    // log level.
    static const base::LogLevel __log_level = base::LOG_WARN_LEVEL;
    // log file name .
    static const std::string __log_file_name = "HudpLog";
    // open log print.
    static const bool __open_log = true;
}

#endif
