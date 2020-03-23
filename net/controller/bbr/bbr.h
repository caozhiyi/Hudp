#ifndef HEADER_NET_CONTROLLER_BBR_BBR
#define HEADER_NET_CONTROLLER_BBR_BBR

#include "CommonType.h"
#include "../OsCommon.h"
#include "../TcpCommon.h"

namespace hudp {

 /* Bottleneck Bandwidth and RTT (BBR) congestion control
 *
 * BBR congestion control computes the sending rate based on the delivery
 * rate (throughput) estimated from ACKs. In a nutshell:
 *
 *   On each ACK, update our model of the network path:
 *      bottleneck_bandwidth = windowed_max(delivered / elapsed, 10 round trips)
 *      min_rtt = windowed_min(rtt, 10 seconds)
 *   pacing_rate = pacing_gain * bottleneck_bandwidth
 *   cwnd = max(cwnd_gain * bottleneck_bandwidth * min_rtt, 4)
 *
 * The core algorithm does not react directly to packet losses or delays,
 * although BBR may adjust the size of next send per ACK when loss is
 * observed, or adjust the sending rate if it estimates there is a
 * traffic policer, in order to keep the drop rate reasonable.
 *
 * Here is a state transition diagram for BBR:
 *
 *             |
 *             V
 *    +---> STARTUP  ----+
 *    |        |         |
 *    |        V         |
 *    |      DRAIN   ----+
 *    |        |         |
 *    |        V         |
 *    +---> PROBE_BW ----+
 *    |      ^    |      |
 *    |      |    |      |
 *    |      +----+      |
 *    |                  |
 *    +---- PROBE_RTT <--+
 *
 * A BBR flow starts in STARTUP, and ramps up its sending rate quickly.
 * When it estimates the pipe is full, it enters DRAIN to drain the queue.
 * In steady state a BBR flow only uses PROBE_BW and PROBE_RTT.
 * A long-lived BBR flow spends the vast majority of its time remaining
 * (repeatedly) in PROBE_BW, fully probing and utilizing the pipe's bandwidth
 * in a fair manner, with a small, bounded queue. *If* a flow has been
 * continuously sending for the entire min_rtt window, and hasn't seen an RTT
 * sample that matches or decreases its min_rtt estimate for 10 seconds, then
 * it briefly enters PROBE_RTT to cut inflight to a minimum value to re-probe
 * the path's two-way propagation delay (min_rtt). When exiting PROBE_RTT, if
 * we estimated that we reached the full bw of the pipe then we enter PROBE_BW;
 * otherwise we enter STARTUP to try to fill the pipe.
 *
 * BBR is described in detail in:
 *   "BBR: Congestion-Based Congestion Control",
 *   Neal Cardwell, Yuchung Cheng, C. Stephen Gunn, Soheil Hassas Yeganeh,
 *   Van Jacobson. ACM Queue, Vol. 14 No. 5, September-October 2016.
 *
 * There is a public e-mail list for discussing BBR development and testing:
 *   https://groups.google.com/forum/#!forum/bbr-dev
 *
 * NOTE: BBR might be used with the fq qdisc ("man tc-fq") with pacing enabled,
 * otherwise TCP stack falls back to an internal pacing using one high
 * resolution timer per TCP socket and may use more resources.
 */

 /* Scale factor for rate in pkt/uSec unit to avoid truncation in bandwidth
  * estimation. The rate unit ~= (1500 bytes / 1 usec / 2^24) ~= 715 bps.
  * This handles bandwidths from 0.06pps (715bps) to 256Mpps (3Tbps) in a uint32_t.
  * Since the minimum window is >=4 packets, the lower bound isn't
  * an issue. The upper bound isn't an issue with existing technologies.
  */
#define BW_SCALE 24
#define BW_UNIT (1 << BW_SCALE)

#define BBR_SCALE 8	/* scaling factor for fractions in BBR (e.g. gains) */
#define BBR_UNIT (1 << BBR_SCALE)

    /* BBR has the following modes for deciding how fast to send: */
    enum bbr_mode {
        BBR_STARTUP,	/* ramp up sending rate rapidly to fill pipe */
        BBR_DRAIN,	/* drain any queue created during startup */
        BBR_PROBE_BW,	/* discover, share bw: pace around estimated bw */
        BBR_PROBE_RTT,	/* cut inflight to min to probe min_rtt */
    };

#define CYCLE_LEN	8	/* number of phases in a pacing gain cycle  发送速度增益因子周期内的阶段数*/

    /* Window length of bw filter (in rounds): */
    static const int bbr_bw_rtts = CYCLE_LEN + 2;
    /* Window length of min_rtt filter (in sec): */
    // 最小rtt采集时间窗口 10s
    static const uint32_t bbr_min_rtt_win_sec = 10;
    /* Minimum time (in ms) spent at bbr_cwnd_min_target in BBR_PROBE_RTT mode: */
    // BBR_PROBE_RTT 模式下在bbr_cwnd_min_target 上花费的最短时间（毫秒）
    static const uint32_t bbr_probe_rtt_mode_ms = 200;
    /* Skip TSO below the following bandwidth (bits/sec): */
    static const int bbr_min_tso_rate = 1200000;

    /* We use a high_gain value of 2/ln(2) because it's the smallest pacing gain
     * that will allow a smoothly increasing pacing rate that will double each RTT
     * and send the same number of packets per RTT that an un-paced, slow-starting
     * Reno or CUBIC flow would:
     */
     // 增益因子上限值
    static const int bbr_high_gain = BBR_UNIT * 2885 / 1000 + 1;
    /* The pacing gain of 1/high_gain in BBR_DRAIN is calculated to typically drain
     * the queue created in BBR_STARTUP in a single round:
     */
    static const int bbr_drain_gain = BBR_UNIT * 1000 / 2885;
    /* The gain for deriving steady-state cwnd tolerates delayed/stretched ACKs: */
    static const int bbr_cwnd_gain = BBR_UNIT * 2;
    /* The pacing_gain values for the PROBE_BW gain cycle, to discover/share bw: */
    static const int bbr_pacing_gain[] = {
        BBR_UNIT * 5 / 4,	/* probe for more available bw */
        BBR_UNIT * 3 / 4,	/* drain queue and/or yield bw to other flows */
        BBR_UNIT, BBR_UNIT, BBR_UNIT,	/* cruise at 1.0*bw to utilize pipe, */
        BBR_UNIT, BBR_UNIT, BBR_UNIT	/* without creating excess queue... */
    };
    /* Randomize the starting gain cycling phase over N phases: */
    static const uint32_t bbr_cycle_rand = 7;

    /* Try to keep at least this many packets in flight, if things go smoothly. For
     * smooth functioning, a sliding window protocol ACKing every other packet
     * needs at least 4 packets in flight:
     * 如果一切顺利的话，试着在飞行中至少保留这么多包裹。为了
     * 平滑运行，一个滑动窗口协议在飞行中每一个包需要至少4个包：
     */
    static const uint32_t bbr_cwnd_min_target = 4;

    /* To estimate if BBR_STARTUP mode (i.e. high_gain) has filled pipe... */
    /* If bw has increased significantly (1.25x), there may be more bw available: */
    static const uint32_t bbr_full_bw_thresh = BBR_UNIT * 5 / 4;
    /* But after 3 rounds w/o significant bw growth, estimate pipe is full: */
    static const uint32_t bbr_full_bw_cnt = 3;

    /* "long-term" ("LT") bandwidth estimator parameters... */
    /* The minimum number of rounds in an LT bw sampling interval: */
    static const uint32_t bbr_lt_intvl_min_rtts = 4;
    /* If lost/delivered ratio > 20%, interval is "lossy" and we may be policed: */
    static const uint32_t bbr_lt_loss_thresh = 50;
    /* If 2 intervals have a bw ratio <= 1/8, their bw is "consistent": */
    static const uint32_t bbr_lt_bw_ratio = BBR_UNIT / 8;
    /* If 2 intervals have a bw diff <= 4 Kbit/sec their bw is "consistent": */
    static const uint32_t bbr_lt_bw_diff = 4000 / 8;
    /* If we estimate we're policed, use lt_bw for this many round trips: */
    static const uint32_t bbr_lt_bw_max_rtts = 48;

    /* BBR congestion control block */
    class CBbr {
    private:
        uint32_t min_rtt_us;	            /* min RTT in min_rtt_win_sec window, rtt采集时间内最小rtt */
        uint32_t min_rtt_stamp;	            /* timestamp of min_rtt_us, min rtt 采集时间戳*/
        uint32_t probe_rtt_done_stamp;      /* end time for BBR_PROBE_RTT mode ， BBR_PROBE_RTT结束时间戳*/
        struct minmax bw;	                /* Max recent delivery rate in pkts/uS << 24 */
        uint32_t rtt_cnt;	                /* count of packet-timed rounds elapsed */
        uint32_t next_rtt_delivered;        /* scb->tx.delivered at end of round, 在回合结束时发送的数据量 */
        uint32_t cycle_mstamp;	            /* time of this cycle phase start 本轮周期开始时间*/
        uint32_t mode : 3,		            /* current bbr_mode in state machine */
                 prev_ca_state : 3,         /* CA state on previous ACK */
                 packet_conservation : 1,   /* use packet conservation? */
                 round_start : 1,	        /* start of packet-timed tx->ack round? */
                 idle_restart : 1,	        /* restarting after idle? 空闲后重新启动 */
                 probe_rtt_round_done : 1,  /* a BBR_PROBE_RTT round at 4 pkts? BBR_PROBE_RTT轮内4个pkts? */
                 unused : 13,
                 lt_is_sampling : 1,        /* taking long-term ("LT") samples now? */
                 lt_rtt_cnt : 7,	        /* round trips in long-term interval */
                 lt_use_bw : 1;	            /* use lt_bw as our bw estimate? 使用lt_bw作为我们的bw估计值*/
        uint32_t lt_bw;		                /* LT est delivery rate in pkts/uS << 24 */
        uint32_t lt_last_delivered;         /* LT intvl start: tp->delivered */
        uint32_t lt_last_stamp;	            /* LT intvl start: tp->delivered_mstamp */
        uint32_t lt_last_lost;	            /* LT intvl start: tp->lost */
        uint32_t pacing_gain : 10,	        /* current gain for setting pacing rate，当前发送速率的增益因子 */
                 cwnd_gain : 10,	        /* current gain for setting cwnd ， 当前发送窗体的增益因子*/
                 full_bw_reached : 1,       /* reached full bw in Startup? 起始阶段达到了最大带宽？ */
                 full_bw_cnt : 2,	        /* number of rounds without large bw gains */
                 cycle_idx : 3,	            /* current index in pacing_gain cycle array， pacing_gain周期内的当前索引  */
                 has_seen_rtt : 1,          /* have we seen an RTT sample yet? */
                 unused_b : 5;
        uint32_t prior_cwnd;	            /* prior cwnd upon entering loss recovery  在进入损失恢复之前，最大发送窗体*/
        uint32_t full_bw;	                /* recent bw, to estimate if pipe is full， 最近的bw，用于估计管道是否已满 */

    public:
        /* Do we estimate that STARTUP filled the pipe? */
        bool bbr_full_bw_reached() { return full_bw_reached; }
        
        /* Return the windowed max recent bandwidth sample, in pkts/uS << BW_SCALE. */
        uint32_t bbr_max_bw() { return minmax_get(&bw); }

        /* Return the estimated bandwidth of the path, in pkts/uS << BW_SCALE. */
        uint32_t bbr_bw() { return lt_use_bw ? lt_bw : bbr_max_bw(); }

        /* Return rate in bytes per second, optionally with a gain.
         * The order here is chosen carefully to avoid overflow of u64. This should
         * work for input rates of up to 2.9Tbit/sec and gain of 2.89x.
         */
        uint64_t bbr_rate_bytes_per_sec(uint64_t rate, int gain) {
            // use default mss
            unsigned int mss = TCP_MSS_DEFAULT;

            rate *= mss;
            rate *= gain;
            rate >>= BBR_SCALE;
            rate *= USEC_PER_SEC;
            return rate >> BW_SCALE;
        }

        /* Convert a BBR bw and gain factor to a pacing rate in bytes per second. */
        uint32_t bbr_bw_to_pacing_rate(uint32_t bw, int gain) {
            uint64_t rate = bw;

            rate = bbr_rate_bytes_per_sec(rate, gain);
            rate = (uint64_t)rate < (uint64_t)(MAX_PACING_RATE) ? (uint64_t)rate : (uint64_t)(MAX_PACING_RATE);
            return rate;
        }

        /* Initialize pacing rate to: high_gain * init_cwnd / RTT. */
        void bbr_init_pacing_rate_from_rtt(uint32_t rtt, uint32_t send_wnd, uint32_t& pacing_rate) {
            uint32_t bw;
            uint32_t rtt_us;

            if (rtt) {		/* any RTT sample yet? */
                rtt_us = rtt > 1U ? rtt : 1U;
                has_seen_rtt = 1;

            } else {			 /* no RTT sample yet */
                rtt_us = USEC_PER_MSEC;	 /* use nominal default RTT */
            }
            bw = send_wnd * BW_UNIT;
            bw = bw / rtt_us;
            pacing_rate = bbr_bw_to_pacing_rate(bw, bbr_high_gain);
        }


        /* Pace using current bw estimate and a gain factor. In order to help drive the
         * network toward lower queues while maintaining high utilization and low
         * latency, the average pacing rate aims to be slightly (~1%) lower than the
         * estimated bandwidth. This is an important aspect of the design. In this
         * implementation this slightly lower pacing rate is achieved implicitly by not
         * including link-layer headers in the packet size used for the pacing rate.
         */
        void bbr_set_pacing_rate(uint32_t rtt, uint32_t send_wnd, uint32_t bw, int gain, uint32_t& pacing_rate) {
            uint32_t rate = bbr_bw_to_pacing_rate(bw, gain);

            if (has_seen_rtt && rtt)
                bbr_init_pacing_rate_from_rtt(rtt, send_wnd, pacing_rate);
            if (bbr_full_bw_reached() || rate > pacing_rate)
                pacing_rate = rate;
        }

        void bbr_reset_startup_mode() {
            // 设置bbr 到 BBR_STARTUP 模式 发送速度增益因子和窗体增益因子设为上限值
            mode = BBR_STARTUP;
            pacing_gain = bbr_high_gain;
            cwnd_gain = bbr_high_gain;
        }

        void bbr_reset_probe_bw_mode() {
            // 设置bbr 到 BBR_PROBE_BW 模式，减小 增益因子
            mode = BBR_PROBE_BW;
            pacing_gain = BBR_UNIT;
            cwnd_gain = bbr_cwnd_gain;
            // TODO
            //cycle_idx = CYCLE_LEN - 1 - prandom_u32_max(bbr_cycle_rand);
            //bbr_advance_cycle_phase(sk);	/* flip to next phase of gain cycle */
        }

        void bbr_reset_mode()  {
            // 起始阶段未达到最大带宽
            if (!bbr_full_bw_reached())
                // 设置为 BBR_STARTUP
                bbr_reset_startup_mode();
            else
                // 设置为 BBR_PROBE_BW
                bbr_reset_probe_bw_mode();
        }

        /* override sysctl_tcp_min_tso_segs */
        uint32_t bbr_min_tso_segs(uint32_t pacing_rate) {
            return pacing_rate < (bbr_min_tso_rate >> 3) ? 1 : 2;
        }

        /* Save "last known good" cwnd so we can restore it after losses or PROBE_RTT */
        void bbr_save_cwnd(uint32_t send_wnd) {
            if (prev_ca_state < TCP_CA_Recovery && mode != BBR_PROBE_RTT)
                prior_cwnd = send_wnd;  /* this cwnd is good enough */
            else  /* loss recovery or BBR_PROBE_RTT have temporarily cut cwnd */
                prior_cwnd = prior_cwnd > send_wnd ? prior_cwnd : send_wnd;
        }

        // 通过有效的bw周期性的计算 gain
        // 带宽探测阶段利用增益系数数组探测带宽．
        /* Gain cycling: cycle pacing gain to converge to fair share of available bw. */
        void bbr_update_cycle_phase(struct sock *sk,
            const struct rate_sample *rs) {
            // 处于BBR_PROBE_BW 阶段， 
            if (mode == BBR_PROBE_BW && bbr_is_next_cycle_phase(sk, rs))
                bbr_advance_cycle_phase(sk);
        }

    };


    static void bbr_check_probe_rtt_done(struct sock *sk);

    static uint32_t bbr_tso_segs_goal(struct sock *sk)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        uint32_t segs, bytes;

        /* Sort of tcp_tso_autosize() but ignoring
         * driver provided sk_gso_max_size.
         */
        bytes = min_t(uint32_t, sk->sk_pacing_rate >> sk->sk_pacing_shift,
            GSO_MAX_SIZE - 1 - MAX_TCP_HEADER);
        segs = max_t(uint32_t, bytes / tp->mss_cache, bbr_min_tso_segs(sk));

        return min(segs, 0x7FU);
    }

    

    static void bbr_cwnd_event(struct sock *sk, enum tcp_ca_event event)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);

        if (event == CA_EVENT_TX_START && tp->app_limited) {
            bbr->idle_restart = 1;
            /* Avoid pointless buffer overflows: pace at est. bw if we don't
             * need more speed (we're restarting from idle and app-limited).
             */
            if (bbr->mode == BBR_PROBE_BW)
                bbr_set_pacing_rate(sk, bbr_bw(sk), BBR_UNIT);
            else if (bbr->mode == BBR_PROBE_RTT)
                bbr_check_probe_rtt_done(sk);
        }
    }

    /* Find target cwnd. Right-size the cwnd based on min RTT and the
     * estimated bottleneck bandwidth:
     *
     * cwnd = bw * min_rtt * gain = BDP * gain
     *
     * The key factor, gain, controls the amount of queue. While a small gain
     * builds a smaller queue, it becomes more vulnerable to noise in RTT
     * measurements (e.g., delayed ACKs or other ACK compression effects). This
     * noise may cause BBR to under-estimate the rate.
     *
     * To achieve full performance in high-speed paths, we budget enough cwnd to
     * fit full-sized skbs in-flight on both end hosts to fully utilize the path:
     *   - one skb in sending host Qdisc,
     *   - one skb in sending host TSO/GSO engine
     *   - one skb being received by receiver host LRO/GRO/delayed-ACK engine
     * Don't worry, at low rates (bbr_min_tso_rate) this won't bloat cwnd because
     * in such cases tso_segs_goal is 1. The minimum cwnd is 4 packets,
     * which allows 2 outstanding 2-packet sequences, to try to keep pipe
     * full even with ACK-every-other-packet delayed ACKs.
     */
     // 通过 bw 和 gain 计算发送窗体大小
    static uint32_t bbr_target_cwnd(struct sock *sk, uint32_t bw, int gain)
    {
        struct bbr *bbr = inet_csk_ca(sk);
        uint32_t cwnd;
        u64 w;

        /* If we've never had a valid RTT sample, cap cwnd at the initial
         * default. This should only happen when the connection is not using TCP
         * timestamps and has retransmitted all of the SYN/SYNACK/data packets
         * ACKed so far. In this case, an RTO can cut cwnd to 1, in which
         * case we need to slow-start up toward something safe: TCP_INIT_CWND.
         */
         // 没有有效的rtt， 发送初始窗体大小
        if (unlikely(bbr->min_rtt_us == ~0U))	 /* no valid RTT samples yet? */
            return TCP_INIT_CWND;  /* be safe: cap at default initial cwnd*/

        w = (u64)bw * bbr->min_rtt_us;

        /* Apply a gain to the given value, then remove the BW_SCALE shift. */
        cwnd = (((w * gain) >> BBR_SCALE) + BW_UNIT - 1) / BW_UNIT;

        /* Allow enough full-sized skbs in flight to utilize end systems. */
        cwnd += 3 * bbr_tso_segs_goal(sk);

        /* Reduce delayed ACKs by rounding up cwnd to the next even number. */
        // 通过将cwnd舍入到下一个偶数来减少延迟ack
        cwnd = (cwnd + 1) & ~1U;

        /* Ensure gain cycling gets inflight above BDP even for small BDPs. */
        if (bbr->mode == BBR_PROBE_BW && gain > BBR_UNIT)
            cwnd += 2;

        return cwnd;
    }

    /* An optimization in BBR to reduce losses: On the first round of recovery, we
     * follow the packet conservation principle: send P packets per P packets acked.
     * After that, we slow-start and send at most 2*P packets per P packets acked.
     * After recovery finishes, or upon undo, we restore the cwnd we had when
     * recovery started (capped by the target cwnd based on estimated BDP).
     *
     * TODO(ycheng/ncardwell): implement a rate-based approach.
     */
    static bool bbr_set_cwnd_to_recover_or_restore(
        struct sock *sk, const struct rate_sample *rs, uint32_t acked, uint32_t *new_cwnd)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);
        u8 prev_state = bbr->prev_ca_state, state = inet_csk(sk)->icsk_ca_state;
        uint32_t cwnd = tp->snd_cwnd;

        /* An ACK for P pkts should release at most 2*P packets. We do this
         * in two steps. First, here we deduct the number of lost packets.
         * Then, in bbr_set_cwnd() we slow start up toward the target cwnd.
         */
        if (rs->losses > 0)
            cwnd = max_t(s32, cwnd - rs->losses, 1);

        if (state == TCP_CA_Recovery && prev_state != TCP_CA_Recovery) {
            /* Starting 1st round of Recovery, so do packet conservation. */
            bbr->packet_conservation = 1;
            bbr->next_rtt_delivered = tp->delivered;  /* start round now */
            /* Cut unused cwnd from app behavior, TSQ, or TSO deferral: */
            cwnd = tcp_packets_in_flight(tp) + acked;
        }
        else if (prev_state >= TCP_CA_Recovery && state < TCP_CA_Recovery) {
            /* Exiting loss recovery; restore cwnd saved before recovery. */
            cwnd = max(cwnd, bbr->prior_cwnd);
            bbr->packet_conservation = 0;
        }
        bbr->prev_ca_state = state;

        if (bbr->packet_conservation) {
            *new_cwnd = max(cwnd, tcp_packets_in_flight(tp) + acked);
            return true;	/* yes, using packet conservation */
        }
        *new_cwnd = cwnd;
        return false;
    }

    /* Slow-start up toward target cwnd (if bw estimate is growing, or packet loss
     * has drawn us down below target), or snap down to target if we're above it.
     */
    static void bbr_set_cwnd(struct sock *sk, const struct rate_sample *rs,
        uint32_t acked, uint32_t bw, int gain)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);
        uint32_t cwnd = tp->snd_cwnd, target_cwnd = 0;

        if (!acked)
            goto done;  /* no packet fully ACKed; just apply caps */

        if (bbr_set_cwnd_to_recover_or_restore(sk, rs, acked, &cwnd))
            goto done;

        /* If we're below target cwnd, slow start cwnd toward target cwnd. */
        target_cwnd = bbr_target_cwnd(sk, bw, gain);
        if (bbr_full_bw_reached(sk))  /* only cut cwnd if we filled the pipe */
            cwnd = min(cwnd + acked, target_cwnd);
        else if (cwnd < target_cwnd || tp->delivered < TCP_INIT_CWND)
            cwnd = cwnd + acked;
        cwnd = max(cwnd, bbr_cwnd_min_target);

    done:
        tp->snd_cwnd = min(cwnd, tp->snd_cwnd_clamp);	/* apply global cap */
        if (bbr->mode == BBR_PROBE_RTT)  /* drain queue, refresh min_rtt */
            tp->snd_cwnd = min(tp->snd_cwnd, bbr_cwnd_min_target);
    }

    /* End cycle phase if it's time and/or we hit the phase's in-flight target. */
    // bbr is next cycle phase
    /*
        利用增益系数数组pacing_gain[5/4, 3/4, 1, 1, 1, 1, 1, 1]探测带宽
        如果是稳定阶段,pacing_gain=1,时长超过min_rtt_us就进入下一轮
        如果是激进阶段,pacing_gain>1,必须时间够了，且有丢包或inflight>目标窗口才进入下一轮
        如果是排空阶段,pacing_gain<1,时间够了，或者inflight<=目标窗口就进入下一轮
    */
    static bool bbr_is_next_cycle_phase(struct sock *sk,
        const struct rate_sample *rs)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);
        /*
            如果delivered_mstamp-cycle_mstamp>min_rtt_us，
            即这一轮带宽探测时长够了.
            标记is_full_length=true，否则is_full_length=false
        */
        bool is_full_length =
            tcp_stamp_us_delta(tp->delivered_mstamp, bbr->cycle_mstamp) >
            bbr->min_rtt_us;
        uint32_t inflight, bw;

        /* The pacing_gain of 1.0 paces at the estimated bw to try to fully
         * use the pipe without increasing the queue.
         */
        if (bbr->pacing_gain == BBR_UNIT)
            return is_full_length;		/* just use wall clock time */

        inflight = rs->prior_in_flight;  /* what was in-flight before ACK? */
        bw = bbr_max_bw(sk);

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
        if (bbr->pacing_gain > BBR_UNIT)
            return is_full_length &&
            (rs->losses ||  /* perhaps pacing_gain*BDP won't fit */
                inflight >= bbr_target_cwnd(sk, bw, bbr->pacing_gain));

        /* A pacing_gain < 1.0 tries to drain extra queue we added if bw
         * probing didn't find more bw. If inflight falls to match BDP then we
         * estimate queue is drained; persisting would underutilize the pipe.
         */
         /*
             如果处于排空队列周期(pacing_gain=3/4),探测时长够了或者
             inflight小于目标窗口可以提前退出这个周期
         */
        return is_full_length ||
            inflight <= bbr_target_cwnd(sk, bw, BBR_UNIT);
    }

    static void bbr_advance_cycle_phase(struct sock *sk)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);

        bbr->cycle_idx = (bbr->cycle_idx + 1) & (CYCLE_LEN - 1);
        bbr->cycle_mstamp = tp->delivered_mstamp;
        // 随机设置增益因子
        bbr->pacing_gain = bbr->lt_use_bw ? BBR_UNIT :
            bbr_pacing_gain[bbr->cycle_idx];
    }

   

    /* Start a new long-term sampling interval. */
    static void bbr_reset_lt_bw_sampling_interval(struct sock *sk)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);

        bbr->lt_last_stamp = div_u64(tp->delivered_mstamp, USEC_PER_MSEC);
        bbr->lt_last_delivered = tp->delivered;
        bbr->lt_last_lost = tp->lost;
        bbr->lt_rtt_cnt = 0;
    }

    /* Completely reset long-term bandwidth sampling. */
    static void bbr_reset_lt_bw_sampling(struct sock *sk)
    {
        struct bbr *bbr = inet_csk_ca(sk);

        bbr->lt_bw = 0;
        bbr->lt_use_bw = 0;
        bbr->lt_is_sampling = false;
        bbr_reset_lt_bw_sampling_interval(sk);
    }

    /* Long-term bw sampling interval is done. Estimate whether we're policed. */
    static void bbr_lt_bw_interval_done(struct sock *sk, uint32_t bw)
    {
        struct bbr *bbr = inet_csk_ca(sk);
        uint32_t diff;

        if (bbr->lt_bw) {  /* do we have bw from a previous interval? */
            /* Is new bw close to the lt_bw from the previous interval? */
            diff = abs(bw - bbr->lt_bw);
            if ((diff * BBR_UNIT <= bbr_lt_bw_ratio * bbr->lt_bw) ||
                (bbr_rate_bytes_per_sec(sk, diff, BBR_UNIT) <=
                    bbr_lt_bw_diff)) {
                /* All criteria are met; estimate we're policed. */
                bbr->lt_bw = (bw + bbr->lt_bw) >> 1;  /* avg 2 intvls */
                bbr->lt_use_bw = 1;
                bbr->pacing_gain = BBR_UNIT;  /* try to avoid drops */
                bbr->lt_rtt_cnt = 0;
                return;
            }
        }
        bbr->lt_bw = bw;
        bbr_reset_lt_bw_sampling_interval(sk);
    }

    /* Token-bucket traffic policers are common (see "An Internet-Wide Analysis of
     * Traffic Policing", SIGCOMM 2016). BBR detects token-bucket policers and
     * explicitly models their policed rate, to reduce unnecessary losses. We
     * estimate that we're policed if we see 2 consecutive sampling intervals with
     * consistent throughput and high packet loss. If we think we're being policed,
     * set lt_bw to the "long-term" average delivery rate from those 2 intervals.
     */
     // 当看到两次连续采样间隔，吞吐量不变并有大量丢包，就认为有traffic policers主动丢包，进入long-term状态，避免进一步大量丢包
    static void bbr_lt_bw_sampling(struct sock *sk, const struct rate_sample *rs)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);
        uint32_t lost, delivered;
        u64 bw;
        uint32_t t;


        /*
        如果已经进入longterm状态
        如果处于bbr带宽探测阶段，且进入新周期，longterm时间已经超过了48个周期，就重置lt采样，切换到bbr带宽探测阶段
        退出longterm状态
        */
        if (bbr->lt_use_bw) {	/* already using long-term rate, lt_bw? */
            if (bbr->mode == BBR_PROBE_BW && bbr->round_start &&
                ++bbr->lt_rtt_cnt >= bbr_lt_bw_max_rtts) {
                bbr_reset_lt_bw_sampling(sk);    /* stop using lt_bw */
                bbr_reset_probe_bw_mode(sk);  /* restart gain cycling */
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
        if (!bbr->lt_is_sampling) {
            if (!rs->losses)
                return;
            bbr_reset_lt_bw_sampling_interval(sk);
            bbr->lt_is_sampling = true;
        }

        /* To avoid underestimates, reset sampling if we run out of data. */
        /* 如果是应用层数据限制，就重新采样 */
        if (rs->is_app_limited) {
            bbr_reset_lt_bw_sampling(sk);
            return;
        }

        /* 采样周期数需要在(4,16)之间 */
        if (bbr->round_start)
            bbr->lt_rtt_cnt++;	/* count round trips in this interval */
        if (bbr->lt_rtt_cnt < bbr_lt_intvl_min_rtts)
            return;		/* sampling interval needs to be longer */
        if (bbr->lt_rtt_cnt > 4 * bbr_lt_intvl_min_rtts) {
            bbr_reset_lt_bw_sampling(sk);  /* interval is too long */
            return;
        }

        /* End sampling interval when a packet is lost, so we estimate the
         * policer tokens were exhausted. Stopping the sampling before the
         * tokens are exhausted under-estimates the policed rate.
         */
         /* 如果rs->losses==0,即没有丢包，则退出long-term检测 */
        if (!rs->losses)
            return;

        /* Calculate packets lost and delivered in sampling interval. */
        lost = tp->lost - bbr->lt_last_lost;
        delivered = tp->delivered - bbr->lt_last_delivered;
        /* Is loss rate (lost/delivered) >= lt_loss_thresh? If not, wait. */
        if (!delivered || (lost << BBR_SCALE) < bbr_lt_loss_thresh * delivered)
            return;

        /* Find average delivery rate in this sampling interval. */
        t = div_u64(tp->delivered_mstamp, USEC_PER_MSEC) - bbr->lt_last_stamp;
        if ((s32)t < 1)
            return;		/* interval is less than one ms, so wait */
        /* Check if can multiply without overflow */
        if (t >= ~0U / USEC_PER_MSEC) {
            bbr_reset_lt_bw_sampling(sk);  /* interval too long; reset */
            return;
        }
        t *= USEC_PER_MSEC;
        bw = (u64)delivered * BW_UNIT;
        do_div(bw, t);
        bbr_lt_bw_interval_done(sk, bw);
    }

    /* Estimate the bandwidth based on how fast packets are delivered */
    static void bbr_update_bw(struct sock *sk, const struct rate_sample *rs)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);
        u64 bw;

        bbr->round_start = 0;
        // 本次采集样本无效
        if (rs->delivered < 0 || rs->interval_us <= 0)
            return; /* Not a valid observation */

        /* See if we've reached the next RTT */
        // 看我们是否到达下一个RTT
        /* rs->prior_delivered为周期开始处的tp->delivered, bbr->next_rtt_delivered为下一个周期的tp->delivered？？
        如果rs->priorr_delivered>=bbr->next_rtt_deliverd，即到了下一个周期，
    　　更新新的下一个周期的开始处的传输数据next_rtt_delivered和轮数rtt_cnt*/
        if (!before(rs->prior_delivered, bbr->next_rtt_delivered)) {
            bbr->next_rtt_delivered = tp->delivered;
            bbr->rtt_cnt++;
            bbr->round_start = 1;
            bbr->packet_conservation = 0;
        }

        bbr_lt_bw_sampling(sk, rs);

        /* Divide delivered by the interval to find a (lower bound) bottleneck
         * bandwidth sample. Delivered is in packets and interval_us in uS and
         * ratio will be <<1 for most connections. So delivered is first scaled.
         */
         // 计算bw 传输数量/时间
        bw = (u64)rs->delivered * BW_UNIT;
        do_div(bw, rs->interval_us);

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
        if (!rs->is_app_limited || bw >= bbr_max_bw(sk)) {
            /* Incorporate new sample into our max bw filter. */
            minmax_running_max(&bbr->bw, bbr_bw_rtts, bbr->rtt_cnt, bw);
        }
    }

    /* Estimate when the pipe is full, using the change in delivery rate: BBR
     * estimates that STARTUP filled the pipe if the estimated bw hasn't changed by
     * at least bbr_full_bw_thresh (25%) after bbr_full_bw_cnt (3) non-app-limited
     * rounds. Why 3 rounds: 1: rwin autotuning grows the rwin, 2: we fill the
     * higher rwin, 3: we get higher delivery rate samples. Or transient
     * cross-traffic or radio noise can go away. CUBIC Hystart shares a similar
     * design goal, but uses delay and inter-ACK spacing instead of bandwidth.
     */
     /*
         检测启动阶段带宽是否已经到了最大值．
         如果连续三次检查到带宽增长速度小于bbr_full_bw_thresh(25%),
         就认为pipe满了,带宽到了最大值
     */
    static void bbr_check_full_bw_reached(struct sock *sk,
        const struct rate_sample *rs)
    {
        struct bbr *bbr = inet_csk_ca(sk);
        uint32_t bw_thresh;

        /*
            如果已经标记了带宽满了，或者round_start=0,即还未开始测，或者应用层限制住了，直接退出
        */
        if (bbr_full_bw_reached(sk) || !bbr->round_start || rs->is_app_limited)
            return;

        // bw没有增大 bbr_full_bw_thresh 25%
        /*
            如果bbr现在的最大带宽比最近带宽多了设定的阈值bbr_full_bw_thresh，
            更新最近带宽bbr->full_bw，并把full_bw_cnt清0
            否则full_bw_cnt+1，full_bw_cnt超过3就表示带宽满了
        */
        bw_thresh = (u64)bbr->full_bw * bbr_full_bw_thresh >> BBR_SCALE;
        if (bbr_max_bw(sk) >= bw_thresh) {
            bbr->full_bw = bbr_max_bw(sk);
            bbr->full_bw_cnt = 0;
            return;
        }
        // 则认为pipe已经满
        ++bbr->full_bw_cnt;
        bbr->full_bw_reached = bbr->full_bw_cnt >= bbr_full_bw_cnt;
    }

    /* If pipe is probably full, drain the queue and then enter steady-state. */
    // 如果pipe已经满，则减小发送速度
    // 如果STARTUP状态带宽增到最大，切换到DRAIN状态；如果DRAIN阶段 队列清空，切换到BBR_PROBE_BW状态
    static void bbr_check_drain(struct sock *sk, const struct rate_sample *rs)
    {
        struct bbr *bbr = inet_csk_ca(sk);

        // 如果是BBR_STARTUP阶段pipe变满。则进入BBR_DRAIN阶段
        /*
            如果慢启动阶段带宽满了(窗口不变，速度减小)
            更新状态机到排空状态
            更新pacing_gain
            保持cwnd_gain
        */
        if (bbr->mode == BBR_STARTUP && bbr_full_bw_reached(sk)) {
            bbr->mode = BBR_DRAIN;	/* drain queue we created */
            bbr->pacing_gain = bbr_drain_gain;	/* pace slow to drain 缓慢的减小*/
            bbr->cwnd_gain = bbr_high_gain;	/* maintain cwnd 保持发送窗体大小*/
            tcp_sk(sk)->snd_ssthresh =
                bbr_target_cwnd(sk, bbr_max_bw(sk), BBR_UNIT);
        }	/* fall through to check if in-flight is already small: */

        // 如果是BBR_DRAIN阶段
        /*
            如果排空阶段inflight<=target，即队列已经清空，切换到带宽探测状态机
        */
        if (bbr->mode == BBR_DRAIN &&
            // 已经发送的数据量
            tcp_packets_in_flight(tcp_sk(sk)) <=
            // 目标窗体大小
            bbr_target_cwnd(sk, bbr_max_bw(sk), BBR_UNIT))
            bbr_reset_probe_bw_mode(sk);  /* we estimate queue is drained 我们估计队列已耗尽*/
    }

    static void bbr_check_probe_rtt_done(struct sock *sk)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);

        // probe_rtt_done_stamp未结束
        if (!(bbr->probe_rtt_done_stamp &&
            after(tcp_jiffies32, bbr->probe_rtt_done_stamp)))
            return;

        // 等待一段时间，直到探测结束
        bbr->min_rtt_stamp = tcp_jiffies32;  /* wait a while until PROBE_RTT */
        // 设置发送窗体大小
        tp->snd_cwnd = max(tp->snd_cwnd, bbr->prior_cwnd);
        bbr_reset_mode(sk);
    }

    /* The goal of PROBE_RTT mode is to have BBR flows cooperatively and
     * periodically drain the bottleneck queue, to converge to measure the true
     * min_rtt (unloaded propagation delay). This allows the flows to keep queues
     * small (reducing queuing delay and packet loss) and achieve fairness among
     * BBR flows.
     *
     * The min_rtt filter window is 10 seconds. When the min_rtt estimate expires,
     * we enter PROBE_RTT mode and cap the cwnd at bbr_cwnd_min_target=4 packets.
     * After at least bbr_probe_rtt_mode_ms=200ms and at least one packet-timed
     * round trip elapsed with that flight size <= 4, we leave PROBE_RTT mode and
     * re-enter the previous mode. BBR uses 200ms to approximately bound the
     * performance penalty of PROBE_RTT's cwnd capping to roughly 2% (200ms/10s).
     *
     * Note that flows need only pay 2% if they are busy sending over the last 10
     * seconds. Interactive applications (e.g., Web, RPCs, video chunks) often have
     * natural silences or low-rate periods within 10 seconds where the rate is low
     * enough for long enough to drain its queue in the bottleneck. We pick up
     * these min RTT measurements opportunistically with our min_rtt filter. :-)
     */
     // 检测是否该进入rtt探测状态以及相应参数更新
    static void bbr_update_min_rtt(struct sock *sk, const struct rate_sample *rs)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);
        bool filter_expired;

        /* Track min RTT seen in the min_rtt_win_sec filter window: */
        // 当前的最小rtt采集时间戳是否已经超过 10s 的时间窗口
        /*
            最小rtt有效时间为bbr_min_rtt_win_sec*HZ, 即10s, 如果有效时间过了或者新采的rtt更小，
            更新最小rtt大小和最小rtt发生的时间
        */
        filter_expired = after(tcp_jiffies32,
            bbr->min_rtt_stamp + bbr_min_rtt_win_sec * HZ);
        // 本次采集到rtt时间， 且比当前的最小rtt小，或者前最小rtt采集时间戳已经过期，本次ack不是延迟确认的ack
        if (rs->rtt_us >= 0 &&
            (rs->rtt_us <= bbr->min_rtt_us ||
            (filter_expired && !rs->is_ack_delayed))) {
            bbr->min_rtt_us = rs->rtt_us;
            bbr->min_rtt_stamp = tcp_jiffies32;
        }

        // filter_expired已经过去
        /*
         如果开启了rtt探测功能，且最小rtt有效时间过了(也可以理解为rtt探测周期到了)，
         且idle_restart==0（不是从空闲状态重启的），且当前不处在rtt探测状态BBR_PROBE_RTT：
         设置状态机为BBR_PROBE_RTT，减小发送速度和发送窗口，保留之前窗口用来恢复，
         rtt探测结束时间标记为无效值0（后面会设置具体有效值）
        */
        if (bbr_probe_rtt_mode_ms > 0 && filter_expired &&
            !bbr->idle_restart && bbr->mode != BBR_PROBE_RTT) {
            // 进入BBR_PROBE_RTT模式
            bbr->mode = BBR_PROBE_RTT;  /* dip, drain queue */
            bbr->pacing_gain = BBR_UNIT;
            bbr->cwnd_gain = BBR_UNIT;
            bbr_save_cwnd(sk);  /* note cwnd so we can restore it */
            bbr->probe_rtt_done_stamp = 0;
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
        if (bbr->mode == BBR_PROBE_RTT) {
            /* Ignore low rate samples during this mode. */
            // 在此模式下忽略低速率采样
            tp->app_limited =
                (tp->delivered + tcp_packets_in_flight(tp)) ? : 1;
            /* Maintain min packets in flight for max(200 ms, 1 round). */
            // 在飞行中保持最短数据包（200毫秒，1轮）
            if (!bbr->probe_rtt_done_stamp &&
                tcp_packets_in_flight(tp) <= bbr_cwnd_min_target) {
                bbr->probe_rtt_done_stamp = tcp_jiffies32 +
                    msecs_to_jiffies(bbr_probe_rtt_mode_ms);
                bbr->probe_rtt_round_done = 0;
                bbr->next_rtt_delivered = tp->delivered;
            }
            else if (bbr->probe_rtt_done_stamp) {
                if (bbr->round_start)
                    bbr->probe_rtt_round_done = 1;
                if (bbr->probe_rtt_round_done)
                    bbr_check_probe_rtt_done(sk);
            }
        }
        /* Restart after idle ends only once we process a new S/ACK for data */
        if (rs->delivered > 0)
            bbr->idle_restart = 0;
    }

    static void bbr_update_model(struct sock *sk, const struct rate_sample *rs)
    {
        //更新当前最大bw
        bbr_update_bw(sk, rs);
        // 更新增益因子
        bbr_update_cycle_phase(sk, rs);
        // 检测pipe已经满
        bbr_check_full_bw_reached(sk, rs);
        // 如果队列已经满， 检测是否应该减小发送速度
        bbr_check_drain(sk, rs);
        // 更新最小rtt时间
        bbr_update_min_rtt(sk, rs);
    }

    static void bbr_main(struct sock *sk, const struct rate_sample *rs)
    {
        struct bbr *bbr = inet_csk_ca(sk);
        uint32_t bw;

        bbr_update_model(sk, rs);

        bw = bbr_bw(sk);
        // 设置发送速度和窗体大小
        bbr_set_pacing_rate(sk, bw, bbr->pacing_gain);
        bbr_set_cwnd(sk, rs, rs->acked_sacked, bw, bbr->cwnd_gain);
    }

    static void bbr_init(struct sock *sk)
    {
        struct tcp_sock *tp = tcp_sk(sk);
        struct bbr *bbr = inet_csk_ca(sk);

        bbr->prior_cwnd = 0;
        tp->snd_ssthresh = TCP_INFINITE_SSTHRESH;
        bbr->rtt_cnt = 0;
        bbr->next_rtt_delivered = 0;
        bbr->prev_ca_state = TCP_CA_Open;
        bbr->packet_conservation = 0;

        bbr->probe_rtt_done_stamp = 0;
        bbr->probe_rtt_round_done = 0;
        bbr->min_rtt_us = tcp_min_rtt(tp);
        bbr->min_rtt_stamp = tcp_jiffies32;

        minmax_reset(&bbr->bw, bbr->rtt_cnt, 0);  /* init max bw to 0 */

        bbr->has_seen_rtt = 0;
        bbr_init_pacing_rate_from_rtt(sk);

        bbr->round_start = 0;
        bbr->idle_restart = 0;
        bbr->full_bw_reached = 0;
        bbr->full_bw = 0;
        bbr->full_bw_cnt = 0;
        bbr->cycle_mstamp = 0;
        bbr->cycle_idx = 0;
        bbr_reset_lt_bw_sampling(sk);
        bbr_reset_startup_mode(sk);

        cmpxchg(&sk->sk_pacing_status, SK_PACING_NONE, SK_PACING_NEEDED);
    }

    static uint32_t bbr_sndbuf_expand(struct sock *sk)
    {
        /* Provision 3 * cwnd since BBR may slow-start even during recovery. */
        return 3;
    }

    /* In theory BBR does not need to undo the cwnd since it does not
     * always reduce cwnd on losses (see bbr_main()). Keep it for now.
     */
    static uint32_t bbr_undo_cwnd(struct sock *sk)
    {
        struct bbr *bbr = inet_csk_ca(sk);

        bbr->full_bw = 0;   /* spurious slow-down; reset full pipe detection */
        bbr->full_bw_cnt = 0;
        bbr_reset_lt_bw_sampling(sk);
        return tcp_sk(sk)->snd_cwnd;
    }

    /* Entering loss recovery, so save cwnd for when we exit or undo recovery. */
    static uint32_t bbr_ssthresh(struct sock *sk)
    {
        bbr_save_cwnd(sk);
        return tcp_sk(sk)->snd_ssthresh;
    }

    static size_t bbr_get_info(struct sock *sk, uint32_t ext, int *attr,
        union tcp_cc_info *info)
    {
        if (ext & (1 << (INET_DIAG_BBRINFO - 1)) ||
            ext & (1 << (INET_DIAG_VEGASINFO - 1))) {
            struct tcp_sock *tp = tcp_sk(sk);
            struct bbr *bbr = inet_csk_ca(sk);
            u64 bw = bbr_bw(sk);

            bw = bw * tp->mss_cache * USEC_PER_SEC >> BW_SCALE;
            memset(&info->bbr, 0, sizeof(info->bbr));
            info->bbr.bbr_bw_lo = (uint32_t)bw;
            info->bbr.bbr_bw_hi = (uint32_t)(bw >> 32);
            info->bbr.bbr_min_rtt = bbr->min_rtt_us;
            info->bbr.bbr_pacing_gain = bbr->pacing_gain;
            info->bbr.bbr_cwnd_gain = bbr->cwnd_gain;
            *attr = INET_DIAG_BBRINFO;
            return sizeof(info->bbr);
        }
        return 0;
    }

    static void bbr_set_state(struct sock *sk, u8 new_state)
    {
        struct bbr *bbr = inet_csk_ca(sk);

        if (new_state == TCP_CA_Loss) {
            struct rate_sample rs = { .losses = 1 };

            bbr->prev_ca_state = TCP_CA_Loss;
            bbr->full_bw = 0;
            bbr->round_start = 1;	/* treat RTO like end of a round */
            bbr_lt_bw_sampling(sk, &rs);
        }
    }

    static struct tcp_congestion_ops tcp_bbr_cong_ops __read_mostly = {
        .flags = TCP_CONG_NON_RESTRICTED,
        .name = "bbr",
        .owner = THIS_MODULE,
        .init = bbr_init,
        .cong_control = bbr_main,
        .sndbuf_expand = bbr_sndbuf_expand,
        .undo_cwnd = bbr_undo_cwnd,
        .cwnd_event = bbr_cwnd_event,
        .ssthresh = bbr_ssthresh,
        .min_tso_segs = bbr_min_tso_segs,
        .get_info = bbr_get_info,
        .set_state = bbr_set_state,
    };

    static int __init bbr_register(void)
    {
        BUILD_BUG_ON(sizeof(struct bbr) > ICSK_CA_PRIV_SIZE);
        return tcp_register_congestion_control(&tcp_bbr_cong_ops);
    }

    static void __exit bbr_unregister(void)
    {
        tcp_unregister_congestion_control(&tcp_bbr_cong_ops);
    }

    module_init(bbr_register);
    module_exit(bbr_unregister);

    MODULE_AUTHOR("Van Jacobson <vanj@google.com>");
    MODULE_AUTHOR("Neal Cardwell <ncardwell@google.com>");
    MODULE_AUTHOR("Yuchung Cheng <ycheng@google.com>");
    MODULE_AUTHOR("Soheil Hassas Yeganeh <soheil@google.com>");
    MODULE_LICENSE("Dual BSD/GPL");
    MODULE_DESCRIPTION("TCP BBR (Bottleneck Bandwidth and RTT)");

}

#endif