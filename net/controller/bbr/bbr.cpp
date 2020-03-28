#include "bbr.h"

using namespace hudp;

uint64_t CBbr::bbr_rate_bytes_per_sec(uint64_t rate, int gain) {
    // use default mss
    unsigned int mss = TCP_MSS_DEFAULT;

    rate *= mss;
    rate *= gain;
    rate >>= BBR_SCALE;
    rate *= USEC_PER_SEC;
    return rate >> BW_SCALE;
}

uint32_t CBbr::bbr_bw_to_pacing_rate(uint32_t bw, int gain) {
    uint64_t rate = bw;

    rate = bbr_rate_bytes_per_sec(rate, gain);
    rate = (uint64_t)rate < (uint64_t)(MAX_PACING_RATE) ? (uint64_t)rate : (uint64_t)(MAX_PACING_RATE);
    return rate;
}

void CBbr::bbr_init_pacing_rate_from_rtt(uint32_t rtt, uint32_t send_wnd, uint32_t& pacing_rate) {
    uint32_t bw;
    uint32_t rtt_us;

    if (rtt) {		/* any RTT sample yet? */
        rtt_us = rtt > 1U ? rtt : 1U;
        has_seen_rtt = 1;

    }
    else {			 /* no RTT sample yet */
        rtt_us = USEC_PER_MSEC;	 /* use nominal default RTT */
    }
    bw = send_wnd * BW_UNIT;
    bw = bw / rtt_us;
    pacing_rate = bbr_bw_to_pacing_rate(bw, bbr_high_gain);
}

void CBbr::bbr_set_pacing_rate(uint32_t rtt, uint32_t send_wnd, uint32_t bw, int gain, uint32_t& pacing_rate) {
    uint32_t rate = bbr_bw_to_pacing_rate(bw, gain);

    if (has_seen_rtt && rtt)
        bbr_init_pacing_rate_from_rtt(rtt, send_wnd, pacing_rate);
    if (bbr_full_bw_reached() || rate > pacing_rate)
        pacing_rate = rate;
}

void CBbr::bbr_reset_startup_mode() {
    // 设置bbr 到 BBR_STARTUP 模式 发送速度增益因子和窗体增益因子设为上限值
    mode = BBR_STARTUP;
    pacing_gain = bbr_high_gain;
    cwnd_gain = bbr_high_gain;
}

void CBbr::bbr_reset_probe_bw_mode(uint64_t delivered_mstamp) {
    // 设置bbr 到 BBR_PROBE_BW 模式，减小 增益因子
    mode = BBR_PROBE_BW;
    pacing_gain = BBR_UNIT;
    cwnd_gain = bbr_cwnd_gain;
    // TODO
    //cycle_idx = CYCLE_LEN - 1 - prandom_u32_max(bbr_cycle_rand);
    cycle_idx++;
    cycle_idx = cycle_idx % CYCLE_LEN;
    bbr_advance_cycle_phase(delivered_mstamp);	/* flip to next phase of gain cycle */
}

void CBbr::bbr_reset_mode(uint64_t delivered_mstamp) {
    // 起始阶段未达到最大带宽
    if (!bbr_full_bw_reached())
        // 设置为 BBR_STARTUP
        bbr_reset_startup_mode();
    else
        // 设置为 BBR_PROBE_BW
        bbr_reset_probe_bw_mode(delivered_mstamp);
}

uint32_t CBbr::bbr_min_tso_segs(uint32_t pacing_rate) {
    return pacing_rate < (bbr_min_tso_rate >> 3) ? 1 : 2;
}

void CBbr::bbr_save_cwnd(uint32_t send_wnd) {
    if (prev_ca_state < TCP_CA_Recovery && mode != BBR_PROBE_RTT)
        prior_cwnd = send_wnd;  /* this cwnd is good enough */
    else  /* loss recovery or BBR_PROBE_RTT have temporarily cut cwnd */
        prior_cwnd = prior_cwnd > send_wnd ? prior_cwnd : send_wnd;
}

void CBbr::bbr_update_cycle_phase(uint32_t pacing_rate, uint64_t delivered_mstamp, uint32_t inflight, bool loss_pkt) {
    // 处于BBR_PROBE_BW 阶段， 
    if (mode == BBR_PROBE_BW && bbr_is_next_cycle_phase(pacing_rate, delivered_mstamp, inflight, loss_pkt))
        bbr_advance_cycle_phase(delivered_mstamp);
}

uint32_t CBbr::bbr_tso_segs_goal(uint32_t pacing_rate) {
    /* Sort of tcp_tso_autosize() but ignoring
        * driver provided sk_gso_max_size.
        */
        // sk_pacing_shift: scaling factor for TCP Small Queues
        // sk_pacing_shift default 10
    uint32_t bytes = pacing_rate >> 10;
    uint32_t segs = bytes / TCP_MSS_DEFAULT > bbr_min_tso_segs(pacing_rate) ? bytes / TCP_MSS_DEFAULT : bbr_min_tso_segs(pacing_rate);

    return std::min(segs, 0x7FU);
}

uint32_t CBbr::bbr_target_cwnd(uint32_t pacing_rate, uint32_t bw, int gain) {
    uint32_t cwnd;
    uint64_t w;
    /* If we've never had a valid RTT sample, cap cwnd at the initial
        * default. This should only happen when the connection is not using TCP
        * timestamps and has retransmitted all of the SYN/SYNACK/data packets
        * ACKed so far. In this case, an RTO can cut cwnd to 1, in which
        * case we need to slow-start up toward something safe: TCP_INIT_CWND.
        */
        // 没有有效的rtt， 发送初始窗体大小
    if (min_rtt_us == ~0U)	    /* no valid RTT samples yet? */
        return TCP_INIT_CWND;   /* be safe: cap at default initial cwnd*/

    w = (uint64_t)bw * min_rtt_us;

    /* Apply a gain to the given value, then remove the BW_SCALE shift. */
    cwnd = (((w * gain) >> BBR_SCALE) + BW_UNIT - 1) / BW_UNIT;

    /* Allow enough full-sized skbs in flight to utilize end systems. */
    cwnd += 3 * bbr_tso_segs_goal(pacing_rate);

    /* Reduce delayed ACKs by rounding up cwnd to the next even number. */
    // 通过将cwnd舍入到下一个偶数来减少延迟ack
    cwnd = (cwnd + 1) & ~1U;

    /* Ensure gain cycling gets inflight above BDP even for small BDPs. */
    if (mode == BBR_PROBE_BW && gain > BBR_UNIT)
        cwnd += 2;

    return cwnd;
}

void CBbr::bbr_set_cwnd(uint32_t pacing_rate, uint32_t acked, uint32_t bw, int gain, uint32_t& cwnd) {
    uint32_t target_cwnd = 0;

    if (!acked)
        goto done;  /* no packet fully ACKed; just apply caps */

    /* If we're below target cwnd, slow start cwnd toward target cwnd. */
    target_cwnd = bbr_target_cwnd(pacing_rate, bw, gain);
    if (bbr_full_bw_reached())  /* only cut cwnd if we filled the pipe */
        cwnd = std::min(cwnd + acked, target_cwnd);
    else if (cwnd < target_cwnd /*tp->delivered < TCP_INIT_CWND*/)
        cwnd = cwnd + acked;
    cwnd = std::max(cwnd, bbr_cwnd_min_target);

done:
    cwnd = std::min(cwnd, (uint32_t)MAX_SEND_WND);	/* apply global cap */
    if (mode == BBR_PROBE_RTT)  /* drain queue, refresh min_rtt */
        cwnd = std::min(cwnd, bbr_cwnd_min_target);
}

bool CBbr::bbr_is_next_cycle_phase(uint32_t pacing_rate, uint64_t delivered_mstamp, uint32_t inflight, bool loss_pkt) {
    /*
        如果delivered_mstamp-cycle_mstamp>min_rtt_us，
        即这一轮带宽探测时长够了.
        标记is_full_length=true，否则is_full_length=false
    */
    bool is_full_length = delivered_mstamp - cycle_mstamp >
        min_rtt_us;

    uint32_t bw;
    /* The pacing_gain of 1.0 paces at the estimated bw to try to fully
        * use the pipe without increasing the queue.
        */
    if (pacing_gain == BBR_UNIT)
        return is_full_length;		/* just use wall clock time */

    bw = bbr_max_bw();

    /* A pacing_gain > 1.0 probes for bw by trying to raise inflight to at
        * least pacing_gain*BDP; this may take more than min_rtt if min_rtt is
        * small (e.g. on a LAN). We do not persist if packets are lost, since
        * a path with small buffers may not hold that much.
        */
        /*
        如果处于探测更大带宽的周期(pacing_gain=5/4)
        如果没有丢包，
        尝试一直处在该周期直到窗口增加到目标窗口(pacing_gain*BDP)
        */
    if (pacing_gain > BBR_UNIT)
        return is_full_length &&
        (loss_pkt ||  /* perhaps pacing_gain*BDP won't fit */
            inflight >= bbr_target_cwnd(pacing_rate, bw, pacing_gain));

    /* A pacing_gain < 1.0 tries to drain extra queue we added if bw
        * probing didn't find more bw. If inflight falls to match BDP then we
        * estimate queue is drained; persisting would underutilize the pipe.
        */
        /*
            如果处于排空队列周期(pacing_gain=3/4),探测时长够了或者
            inflight小于目标窗口可以提前退出这个周期
        */
    return is_full_length ||
        inflight <= bbr_target_cwnd(pacing_rate, bw, BBR_UNIT);
}

void CBbr::bbr_advance_cycle_phase(uint64_t delivered_mstamp) {
    cycle_idx = (cycle_idx + 1) & (CYCLE_LEN - 1);
    cycle_mstamp = delivered_mstamp;
    // 随机设置增益因子
    pacing_gain = lt_use_bw ? BBR_UNIT :
        bbr_pacing_gain[cycle_idx];
}

void CBbr::bbr_reset_lt_bw_sampling_interval(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost) {
    lt_last_stamp = delivered_mstamp / USEC_PER_MSEC;
    lt_last_delivered = delivered;
    lt_last_lost = lost;
    lt_rtt_cnt = 0;
}

void CBbr::bbr_reset_lt_bw_sampling(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost) {
    lt_bw = 0;
    lt_use_bw = 0;
    lt_is_sampling = false;
    bbr_reset_lt_bw_sampling_interval(delivered_mstamp, delivered, lost);
}

void CBbr::bbr_lt_bw_interval_done(uint32_t bw, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost) {
    uint32_t diff;

    if (lt_bw) {  /* do we have bw from a previous interval? */
        /* Is new bw close to the lt_bw from the previous interval? */
        diff = bw > lt_bw ? bw - lt_bw : lt_bw - bw;
        if ((diff * BBR_UNIT <= bbr_lt_bw_ratio * lt_bw) ||
            (bbr_rate_bytes_per_sec(diff, BBR_UNIT) <=
                bbr_lt_bw_diff)) {
            /* All criteria are met; estimate we're policed. */
            lt_bw = (bw + lt_bw) >> 1;  /* avg 2 intvls */
            lt_use_bw = 1;
            pacing_gain = BBR_UNIT;  /* try to avoid drops */
            lt_rtt_cnt = 0;
            return;
        }
    }
    lt_bw = bw;
    bbr_reset_lt_bw_sampling_interval(delivered_mstamp, delivered, lost);
}

void CBbr::bbr_lt_bw_sampling(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool is_app_limited) {
    uint64_t bw;
    uint32_t t;

    /*
    如果已经进入longterm状态
    如果处于bbr带宽探测阶段，且进入新周期，longterm时间已经超过了48个周期，就重置lt采样，切换到bbr带宽探测阶段
    退出longterm状态
    */
    if (lt_use_bw) {	/* already using long-term rate, lt_bw? */
        if (mode == BBR_PROBE_BW && round_start &&
            ++lt_rtt_cnt >= bbr_lt_bw_max_rtts) {
            bbr_reset_lt_bw_sampling(delivered_mstamp, delivered, lost);    /* stop using lt_bw */
            bbr_reset_probe_bw_mode(delivered_mstamp);  /* restart gain cycling */
        }
        return;
    }

    /* Wait for the first loss before sampling, to let the policer exhaust
        * its tokens and estimate the steady-state rate allowed by the policer.
        * Starting samples earlier includes bursts that over-estimate the bw.
        */
        /*
        如果lt_is_sampling==fasle,即没有采样
        如果rs->losses==0,即没有丢包，则退出long-term检测
        开始一个新的long-term采样．更新采样标签
        */
    if (!lt_is_sampling) {
        if (!lost)
            return;
        bbr_reset_lt_bw_sampling_interval(delivered_mstamp, delivered, lost);
        lt_is_sampling = true;
    }

    /* To avoid underestimates, reset sampling if we run out of data. */
    /* 如果是应用层数据限制，就重新采样 */
    if (is_app_limited) {
        bbr_reset_lt_bw_sampling(delivered_mstamp, delivered, lost);
        return;
    }

    /* 采样周期数需要在(4,16)之间 */
    if (round_start)
        lt_rtt_cnt++;	/* count round trips in this interval */
    if (lt_rtt_cnt < bbr_lt_intvl_min_rtts)
        return;		/* sampling interval needs to be longer */
    if (lt_rtt_cnt > 4 * bbr_lt_intvl_min_rtts) {
        bbr_reset_lt_bw_sampling(delivered_mstamp, delivered, lost);  /* interval is too long */
        return;
    }

    /* End sampling interval when a packet is lost, so we estimate the
        * policer tokens were exhausted. Stopping the sampling before the
        * tokens are exhausted under-estimates the policed rate.
        */
        /* 如果rs->losses==0,即没有丢包，则退出long-term检测 */
    if (!lost)
        return;

    /* Calculate packets lost and delivered in sampling interval. */
    lost = lost - lt_last_lost;
    delivered = delivered - lt_last_delivered;
    /* Is loss rate (lost/delivered) >= lt_loss_thresh? If not, wait. */
    if (!delivered || (lost << BBR_SCALE) < bbr_lt_loss_thresh * delivered)
        return;

    /* Find average delivery rate in this sampling interval. */
    t = delivered_mstamp / USEC_PER_MSEC - lt_last_stamp;
    if (t < 1)
        return;		/* interval is less than one ms, so wait */
    /* Check if can multiply without overflow */
    if (t >= ~0U / USEC_PER_MSEC) {
        bbr_reset_lt_bw_sampling(delivered_mstamp, delivered, lost);  /* interval too long; reset */
        return;
    }
    t *= USEC_PER_MSEC;
    bw = delivered * BW_UNIT;
    bw = bw / t;
    bbr_lt_bw_interval_done(bw, delivered_mstamp, delivered, lost);
}

void CBbr::bbr_update_bw(uint32_t rrt, uint64_t prior_delivered, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool app_limit) {
    uint32_t cur_bw;
    round_start = 0;

    // 本次采集样本无效
    if (delivered < 0 || rrt <= 0)
        return; /* Not a valid observation */

    /* See if we've reached the next RTT */
    // 看我们是否到达下一个RTT
    /* rs->prior_delivered为周期开始处的tp->delivered, bbr->next_rtt_delivered为下一个周期的tp->delivered？？
    如果rs->priorr_delivered>=bbr->next_rtt_deliverd，即到了下一个周期，
        　　更新新的下一个周期的开始处的传输数据next_rtt_delivered和轮数rtt_cnt*/
    if (prior_delivered > next_rtt_delivered) {
        next_rtt_delivered = delivered;
        rtt_cnt++;
        round_start = 1;
        packet_conservation = 0;
    }

    bbr_lt_bw_sampling(delivered_mstamp, delivered, lost, app_limit);

    /* Divide delivered by the interval to find a (lower bound) bottleneck
        * bandwidth sample. Delivered is in packets and interval_us in uS and
        * ratio will be <<1 for most connections. So delivered is first scaled.
        */
        // 计算bw 传输数量/时间
    cur_bw = delivered * BW_UNIT;
    cur_bw = cur_bw / rrt;

    /* If this sample is application-limited, it is likely to have a very
        * low delivered count that represents application behavior rather than
        * the available network rate. Such a sample could drag down estimated
        * bw, causing needless slow-down. Thus, to continue to send at the
        * last measured network rate, we filter out app-limited samples unless
        * they describe the path bw at least as well as our bw model.
        *
        * So the goal during app-limited phase is to proceed with the best
        * network rate no matter how long. We automatically leave this
        * phase when app writes faster than the network can deliver :)
        */
        // 非app限制且比之前观测到的bw还大，更新bw
    if (!app_limit || cur_bw >= bbr_max_bw()) {
        /* Incorporate new sample into our max bw filter. */
        minmax_running_max(&bw, bbr_bw_rtts, rtt_cnt, cur_bw);
    }
}

void CBbr::bbr_check_full_bw_reached(bool app_limite) {
    uint32_t bw_thresh;

    /*
        如果已经标记了带宽满了，或者round_start=0,即还未开始测，或者应用层限制住了，直接退出
    */
    if (bbr_full_bw_reached() || !round_start || app_limite)
        return;

    // bw没有增大 bbr_full_bw_thresh 25%
    /*
        如果bbr现在的最大带宽比最近带宽多了设定的阈值bbr_full_bw_thresh，
        更新最近带宽bbr->full_bw，并把full_bw_cnt清0
        否则full_bw_cnt+1，full_bw_cnt超过3就表示带宽满了
    */
    bw_thresh = (uint64_t)full_bw * bbr_full_bw_thresh >> BBR_SCALE;
    if (bbr_max_bw() >= bw_thresh) {
        full_bw = bbr_max_bw();
        full_bw_cnt = 0;
        return;
    }
    // 则认为pipe已经满
    ++full_bw_cnt;
    full_bw_reached = full_bw_cnt >= bbr_full_bw_cnt;
}

void CBbr::bbr_check_drain(uint64_t delivered_mstamp, uint32_t pacing_rate, uint32_t inflight, uint32_t& snd_ssthresh) {
    // 如果是BBR_STARTUP阶段pipe变满。则进入BBR_DRAIN阶段
    /*
        如果慢启动阶段带宽满了(窗口不变，速度减小)
        更新状态机到排空状态
        更新pacing_gain
        保持cwnd_gain
    */
    if (mode == BBR_STARTUP && bbr_full_bw_reached()) {
        mode = BBR_DRAIN;	/* drain queue we created */
        pacing_gain = bbr_drain_gain;	/* pace slow to drain 缓慢的减小*/
        cwnd_gain = bbr_high_gain;	/* maintain cwnd 保持发送窗体大小*/
        snd_ssthresh = bbr_target_cwnd(pacing_rate, bbr_max_bw(), BBR_UNIT);
    }	/* fall through to check if in-flight is already small: */

    // 如果是BBR_DRAIN阶段
    /*
        如果排空阶段inflight<=target，即队列已经清空，切换到带宽探测状态机
    */
    if (mode == BBR_DRAIN && inflight <= bbr_target_cwnd(pacing_rate, bbr_max_bw(), BBR_UNIT))
        bbr_reset_probe_bw_mode(delivered_mstamp);  /* we estimate queue is drained 我们估计队列已耗尽*/
}

void CBbr::bbr_check_probe_rtt_done(uint64_t delivered_mstamp, uint32_t& send_wnd) {
    // probe_rtt_done_stamp未结束
    if (!(probe_rtt_done_stamp &&
        GetCurTimeStamp() < probe_rtt_done_stamp))
        return;

    // 等待一段时间，直到探测结束
    min_rtt_stamp = GetCurTimeStamp();  /* wait a while until PROBE_RTT */
    // 设置发送窗体大小
    send_wnd = std::max(send_wnd, prior_cwnd);
    bbr_reset_mode(delivered_mstamp);
}

void CBbr::bbr_update_min_rtt(uint64_t delivered_mstamp, uint32_t inflight, uint64_t delivered, uint32_t rtt_us, bool is_ack_delayed, uint32_t& send_wnd) {
    bool filter_expired;

    /* Track min RTT seen in the min_rtt_win_sec filter window: */
    // 当前的最小rtt采集时间戳是否已经超过 10s 的时间窗口
    /*
        最小rtt有效时间为bbr_min_rtt_win_sec*HZ, 即10s, 如果有效时间过了或者新采的rtt更小，
        更新最小rtt大小和最小rtt发生的时间
    */
    filter_expired = GetCurTimeStamp() < min_rtt_stamp + bbr_min_rtt_win_sec * HZ;
    // 本次采集到rtt时间， 且比当前的最小rtt小，或者前最小rtt采集时间戳已经过期，本次ack不是延迟确认的ack
    if (rtt_us >= 0 && (rtt_us <= min_rtt_us || (filter_expired && is_ack_delayed))) {
        min_rtt_us = rtt_us;
        min_rtt_stamp = GetCurTimeStamp();
    }

    // filter_expired已经过去
    /*
        如果开启了rtt探测功能，且最小rtt有效时间过了(也可以理解为rtt探测周期到了)，
        且idle_restart==0（不是从空闲状态重启的），且当前不处在rtt探测状态BBR_PROBE_RTT：
        设置状态机为BBR_PROBE_RTT，减小发送速度和发送窗口，保留之前窗口用来恢复，
        rtt探测结束时间标记为无效值0（后面会设置具体有效值）
    */
    if (bbr_probe_rtt_mode_ms > 0 && filter_expired && !idle_restart && mode != BBR_PROBE_RTT) {
        // 进入BBR_PROBE_RTT模式
        mode = BBR_PROBE_RTT;  /* dip, drain queue */
        pacing_gain = BBR_UNIT;
        cwnd_gain = BBR_UNIT;
        bbr_save_cwnd(send_wnd);  /* note cwnd so we can restore it */
        probe_rtt_done_stamp = 0;
    }
    /*
    如果处于rtt探测状态，更新限制
    如果probe_rtt_done_stamp=0（结束标记无效），且网络中的包少于bbr_cwnd_min_target
        更新rtt探测结束时间,设置probe_rtt_round_done=0(标记rtt探测还没有开始做过),更新下一个rtt的delivered
    否则如果probe_rtt_done_stamp!=0（结束标记有效）
        如果round_start=1
            标记probe_rtt_round_done=1（rtt探测已经开始做了）
        如果rtt探测已经生做过了，而且探测时长到了
            更新最小rtt计算的时间（用于判断有没有过期，以重新进入rtt探测周期）
            标记恢复窗口
            重置模型
    */
    if (mode == BBR_PROBE_RTT) {
        /* Maintain min packets in flight for max(200 ms, 1 round). */
        // 在飞行中保持最短数据包（200毫秒，1轮）
        if (!probe_rtt_done_stamp &&
            inflight <= bbr_cwnd_min_target) {
            probe_rtt_done_stamp = GetCurTimeStamp() + bbr_probe_rtt_mode_ms;
            probe_rtt_round_done = 0;
            next_rtt_delivered = delivered;
        }
        else if (probe_rtt_done_stamp) {
            if (round_start)
                probe_rtt_round_done = 1;
            if (probe_rtt_round_done)
                bbr_check_probe_rtt_done(delivered_mstamp, send_wnd);
        }
    }
    /* Restart after idle ends only once we process a new S/ACK for data */
    if (delivered > 0)
        idle_restart = 0;
}

void CBbr::bbr_update_model(uint32_t pacing_rate, uint32_t inflight, uint32_t rrt,
    uint64_t prior_delivered, uint64_t delivered_mstamp,
    uint32_t delivered, uint32_t lost, bool app_limit, bool is_delay_ack,
    uint32_t& snd_ssthresh, uint32_t& send_wnd) {
    //更新当前最大bw
    bbr_update_bw(rrt, prior_delivered, delivered_mstamp, delivered, lost, app_limit);
    // 更新增益因子
    bbr_update_cycle_phase(pacing_rate, delivered_mstamp, inflight, lost > 0);
    // 检测pipe已经满
    bbr_check_full_bw_reached(app_limit);
    // 如果队列已经满， 检测是否应该减小发送速度
    bbr_check_drain(delivered_mstamp, pacing_rate, inflight, snd_ssthresh);
    // 更新最小rtt时间
    bbr_update_min_rtt(delivered_mstamp, inflight, delivered, rrt, is_delay_ack, send_wnd);
}

void CBbr::bbr_main(uint32_t inflight, uint32_t rrt, uint32_t acked,
    uint64_t prior_delivered, uint64_t delivered_mstamp,
    uint32_t delivered, uint32_t lost, bool app_limit, bool is_delay_ack,
    uint32_t& snd_ssthresh, uint32_t& send_wnd, uint32_t& pacing_rate) {

    uint32_t bw;
    bbr_update_model(pacing_rate, inflight, rrt,
        prior_delivered, delivered_mstamp,
        delivered, lost, app_limit, is_delay_ack,
        snd_ssthresh, send_wnd);

    bw = bbr_bw();
    // 设置发送速度和窗体大小
    bbr_set_pacing_rate(rrt, send_wnd, bw, pacing_gain, pacing_rate);
    bbr_set_cwnd(pacing_rate, acked, bw, pacing_gain, send_wnd);
}

void CBbr::bbr_init() {
    prior_cwnd = 0;
    rtt_cnt = 0;
    next_rtt_delivered = 0;
    prev_ca_state = TCP_CA_Open;
    packet_conservation = 0;

    probe_rtt_done_stamp = 0;
    probe_rtt_round_done = 0;
    min_rtt_us = 0;
    min_rtt_stamp = GetCurTimeStamp();

    minmax_reset(&bw, rtt_cnt, 0);  /* init max bw to 0 */

    has_seen_rtt = 0;

    round_start = 0;
    idle_restart = 0;
    full_bw_reached = 0;
    full_bw = 0;
    full_bw_cnt = 0;
    cycle_mstamp = 0;
    cycle_idx = 0;
    bbr_reset_startup_mode();
}