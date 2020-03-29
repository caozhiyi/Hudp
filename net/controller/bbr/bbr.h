#ifndef HEADER_NET_CONTROLLER_BBR_BBR
#define HEADER_NET_CONTROLLER_BBR_BBR

#include <algorithm>
#include "CommonFunc.h"
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
        uint64_t min_rtt_stamp;	            /* timestamp of min_rtt_us, min rtt 采集时间戳*/
        uint32_t probe_rtt_done_stamp;      /* end time for BBR_PROBE_RTT mode ， BBR_PROBE_RTT结束时间戳*/
        struct minmax bw;	                /* Max recent delivery rate in pkts/uS << 24 本次bw */
        uint32_t rtt_cnt;	                /* count of packet-timed rounds elapsed */
        uint32_t next_rtt_delivered;        /* scb->tx.delivered at end of round, 在回合结束时发送的数据量 */
        uint32_t cycle_mstamp;	            /* time of this cycle phase start 本轮周期开始时间*/
        uint32_t mode : 3,		            /* current bbr_mode in state machine */
                 packet_conservation : 1,   /* use packet conservation? */
                 round_start : 1,	        /* start of packet-timed tx->ack round? */
                 idle_restart : 1,	        /* restarting after idle? 空闲后重新启动 */
                 probe_rtt_round_done : 1,  /* a BBR_PROBE_RTT round at 4 pkts? BBR_PROBE_RTT轮内4个pkts? */
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
                 has_seen_rtt : 1;          /* have we seen an RTT sample yet? */
        uint32_t prior_cwnd;	            /* prior cwnd upon entering loss recovery  在进入损失恢复之前，最大发送窗体*/
        uint32_t full_bw;	                /* recent bw, to estimate if pipe is full， 最近的bw，用于估计管道是否已满 */

    private:
        CBbr() { bbr_init(); }
        ~CBbr() {}
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
        uint64_t bbr_rate_bytes_per_sec(uint64_t rate, int gain);

        /* Convert a BBR bw and gain factor to a pacing rate in bytes per second. */
        uint32_t bbr_bw_to_pacing_rate(uint32_t bw, int gain);

        /* Initialize pacing rate to: high_gain * init_cwnd / RTT. */
        void bbr_init_pacing_rate_from_rtt(uint32_t rtt, uint32_t send_wnd, uint32_t& pacing_rate);


        /* Pace using current bw estimate and a gain factor. In order to help drive the
         * network toward lower queues while maintaining high utilization and low
         * latency, the average pacing rate aims to be slightly (~1%) lower than the
         * estimated bandwidth. This is an important aspect of the design. In this
         * implementation this slightly lower pacing rate is achieved implicitly by not
         * including link-layer headers in the packet size used for the pacing rate.
         */
        void bbr_set_pacing_rate(uint32_t rtt, uint32_t send_wnd, uint32_t bw, int gain, uint32_t& pacing_rate);

        void bbr_reset_startup_mode();

        void bbr_reset_probe_bw_mode(uint64_t delivered_mstamp);

        void bbr_reset_mode(uint64_t delivered_mstamp);

        /* override sysctl_tcp_min_tso_segs */
        uint32_t bbr_min_tso_segs(uint32_t pacing_rate);

        /* Save "last known good" cwnd so we can restore it after losses or PROBE_RTT */
        void bbr_save_cwnd(uint32_t send_wnd);

        // 通过有效的bw周期性的计算 gain
        // 带宽探测阶段利用增益系数数组探测带宽．
        /* Gain cycling: cycle pacing gain to converge to fair share of available bw. */
        void bbr_update_cycle_phase(uint32_t pacing_rate, uint64_t delivered_mstamp, uint32_t inflight, bool loss_pkt);

        uint32_t bbr_tso_segs_goal(uint32_t pacing_rate);

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
        uint32_t bbr_target_cwnd(uint32_t pacing_rate, uint32_t bw, int gain);

        /* Slow-start up toward target cwnd (if bw estimate is growing, or packet loss
         * has drawn us down below target), or snap down to target if we're above it.
        */
        void bbr_set_cwnd(uint32_t pacing_rate, uint32_t acked, uint32_t bw, int gain, uint32_t& cwnd);

        /* End cycle phase if it's time and/or we hit the phase's in-flight target. */
        // bbr is next cycle phase
        /*
        利用增益系数数组pacing_gain[5/4, 3/4, 1, 1, 1, 1, 1, 1]探测带宽
        如果是稳定阶段,pacing_gain=1,时长超过min_rtt_us就进入下一轮
        如果是激进阶段,pacing_gain>1,必须时间够了，且有丢包或inflight>目标窗口才进入下一轮
        如果是排空阶段,pacing_gain<1,时间够了，或者inflight<=目标窗口就进入下一轮
        */
        bool bbr_is_next_cycle_phase(uint32_t pacing_rate, uint64_t delivered_mstamp, uint32_t inflight, bool loss_pkt);

        //设置下一轮增益因子
        void bbr_advance_cycle_phase(uint64_t delivered_mstamp);
        
        /* Start a new long-term sampling interval. */
        void bbr_reset_lt_bw_sampling_interval(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost);

        /* Completely reset long-term bandwidth sampling. */
        void bbr_reset_lt_bw_sampling(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost);

        /* Long-term bw sampling interval is done. Estimate whether we're policed. */
        void bbr_lt_bw_interval_done(uint32_t bw, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost);

        /* Token-bucket traffic policers are common (see "An Internet-Wide Analysis of
        * Traffic Policing", SIGCOMM 2016). BBR detects token-bucket policers and
        * explicitly models their policed rate, to reduce unnecessary losses. We
        * estimate that we're policed if we see 2 consecutive sampling intervals with
        * consistent throughput and high packet loss. If we think we're being policed,
        * set lt_bw to the "long-term" average delivery rate from those 2 intervals.
        */
        // 当看到两次连续采样间隔，吞吐量不变并有大量丢包，就认为有traffic policers主动丢包，进入long-term状态，避免进一步大量丢包
        void bbr_lt_bw_sampling(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool is_app_limited);

        /* Estimate the bandwidth based on how fast packets are delivered */
        void bbr_update_bw(uint32_t rrt, uint64_t prior_delivered, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool app_limit);

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
        void bbr_check_full_bw_reached(bool app_limite);

        /* If pipe is probably full, drain the queue and then enter steady-state. */
        // 如果pipe已经满，则减小发送速度
        // 如果STARTUP状态带宽增到最大，切换到DRAIN状态；如果DRAIN阶段 队列清空，切换到BBR_PROBE_BW状态
        void bbr_check_drain(uint64_t delivered_mstamp, uint32_t pacing_rate, uint32_t inflight, uint32_t& snd_ssthresh);

        void bbr_check_probe_rtt_done(uint64_t delivered_mstamp, uint32_t& send_wnd);

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
        void bbr_update_min_rtt(uint64_t delivered_mstamp, uint32_t inflight, uint64_t delivered, uint32_t rtt_us, bool is_ack_delayed, uint32_t& send_wnd);
        void bbr_init();
        void bbr_update_model(uint32_t pacing_rate, uint32_t inflight, uint32_t rrt,
            uint64_t prior_delivered, uint64_t delivered_mstamp,
            uint32_t delivered, uint32_t lost, bool app_limit, bool is_delay_ack,
            uint32_t& snd_ssthresh, uint32_t& send_wnd);

    public:
        // BBR driven by incoming parameters, get snd_ssthresh, send_wnd and pacint_rate.
        // params:
        // inflight : in flight before this ACK
        // rtt      : nearly observed rtt time
        // acked    : number of packets newly (S)ACKed upon ACK
        // prior_delivered : delivered packets to receiver in recovery
        // delivered: total data packets delivered incl // 到目前为止已经确认的数据包数量
        // delivered_mstamp: time we reached "delivered"// 最后确认数据包时间戳
        // 
        void bbr_main(uint32_t inflight, uint32_t rrt, uint32_t acked,
                      uint64_t prior_delivered, uint64_t delivered_mstamp,
                      uint32_t delivered, uint32_t lost, bool app_limit, bool is_delay_ack,
                      uint32_t& snd_ssthresh, uint32_t& send_wnd, uint32_t& pacing_rate);

    };
}

#endif