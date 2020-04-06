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

#define CYCLE_LEN	8	/* number of phases in a pacing gain cycle  �����ٶ��������������ڵĽ׶���*/

    /* Window length of bw filter (in rounds): */
    static const int bbr_bw_rtts = CYCLE_LEN + 2;
    /* Window length of min_rtt filter (in sec): */
    // ��Сrtt�ɼ�ʱ�䴰�� 10s
    static const uint32_t bbr_min_rtt_win_sec = 10;
    /* Minimum time (in ms) spent at bbr_cwnd_min_target in BBR_PROBE_RTT mode: */
    // BBR_PROBE_RTT ģʽ����bbr_cwnd_min_target �ϻ��ѵ����ʱ�䣨���룩
    static const uint32_t bbr_probe_rtt_mode_ms = 200;
    /* Skip TSO below the following bandwidth (bits/sec): */
    static const int bbr_min_tso_rate = 1200000;

    /* We use a high_gain value of 2/ln(2) because it's the smallest pacing gain
     * that will allow a smoothly increasing pacing rate that will double each RTT
     * and send the same number of packets per RTT that an un-paced, slow-starting
     * Reno or CUBIC flow would:
     */
     // ������������ֵ
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
     * ���һ��˳���Ļ��������ڷ��������ٱ�����ô�������Ϊ��
     * ƽ�����У�һ����������Э���ڷ�����ÿһ������Ҫ����4������
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
        uint32_t min_rtt_us;	            /* min RTT in min_rtt_win_sec window, rtt�ɼ�ʱ������Сrtt */
        uint64_t min_rtt_stamp;	            /* timestamp of min_rtt_us, min rtt �ɼ�ʱ���*/
        uint64_t probe_rtt_done_stamp;      /* end time for BBR_PROBE_RTT mode �� BBR_PROBE_RTT����ʱ���*/
        struct minmax bw;	                /* Max recent delivery rate in pkts/uS << 24 ����bw */
        uint32_t rtt_cnt;	                /* count of packet-timed rounds elapsed */
        uint64_t next_rtt_delivered;        /* scb->tx.delivered at end of round, �ڻغϽ���ʱ���͵������� */
        uint64_t cycle_mstamp;	            /* time of this cycle phase start �������ڿ�ʼʱ��*/
        uint32_t mode : 3,		            /* current bbr_mode in state machine */
                 packet_conservation : 1,   /* use packet conservation? */
                 round_start : 1,	        /* start of packet-timed tx->ack round? */
                 idle_restart : 1,	        /* restarting after idle? ���к��������� */
                 probe_rtt_round_done : 1,  /* a BBR_PROBE_RTT round at 4 pkts? BBR_PROBE_RTT����4��pkts? */
                 lt_is_sampling : 1,        /* taking long-term ("LT") samples now? */
                 lt_rtt_cnt : 7,	        /* round trips in long-term interval */
                 lt_use_bw : 1;	            /* use lt_bw as our bw estimate? ʹ��lt_bw��Ϊ���ǵ�bw����ֵ*/
        uint32_t lt_bw;		                /* LT est delivery rate in pkts/uS << 24 */
        uint32_t lt_last_delivered;         /* LT intvl start: tp->delivered */
        uint64_t lt_last_stamp;	            /* LT intvl start: tp->delivered_mstamp */
        uint32_t lt_last_lost;	            /* LT intvl start: tp->lost */
        uint32_t pacing_gain : 10,	        /* current gain for setting pacing rate����ǰ�������ʵ��������� */
                 cwnd_gain : 10,	        /* current gain for setting cwnd �� ��ǰ���ʹ������������*/
                 full_bw_reached : 1,       /* reached full bw in Startup? ��ʼ�׶δﵽ���������� */
                 full_bw_cnt : 2,	        /* number of rounds without large bw gains */
                 cycle_idx : 3,	            /* current index in pacing_gain cycle array�� pacing_gain�����ڵĵ�ǰ����  */
                 has_seen_rtt : 1;          /* have we seen an RTT sample yet? */
        uint32_t prior_cwnd;	            /* prior cwnd upon entering loss recovery  �ڽ�����ʧ�ָ�֮ǰ������ʹ���*/
        uint32_t full_bw;	                /* recent bw, to estimate if pipe is full�� �����bw�����ڹ��ƹܵ��Ƿ����� */

    private:
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

        // ͨ����Ч��bw�����Եļ��� gain
        // ����̽��׶���������ϵ������̽�������
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
        // ͨ�� bw �� gain ���㷢�ʹ����С
        uint32_t bbr_target_cwnd(uint32_t pacing_rate, uint32_t bw, int gain);

        /* Slow-start up toward target cwnd (if bw estimate is growing, or packet loss
         * has drawn us down below target), or snap down to target if we're above it.
        */
        void bbr_set_cwnd(uint32_t pacing_rate, uint32_t acked, uint32_t bw, int gain, uint32_t& cwnd);

        /* End cycle phase if it's time and/or we hit the phase's in-flight target. */
        // bbr is next cycle phase
        /*
        ��������ϵ������pacing_gain[5/4, 3/4, 1, 1, 1, 1, 1, 1]̽�����
        ������ȶ��׶�,pacing_gain=1,ʱ������min_rtt_us�ͽ�����һ��
        ����Ǽ����׶�,pacing_gain>1,����ʱ�乻�ˣ����ж�����inflight>Ŀ�괰�ڲŽ�����һ��
        ������ſս׶�,pacing_gain<1,ʱ�乻�ˣ�����inflight<=Ŀ�괰�ھͽ�����һ��
        */
        bool bbr_is_next_cycle_phase(uint32_t pacing_rate, uint64_t delivered_mstamp, uint32_t inflight, bool loss_pkt);

        //������һ����������
        void bbr_advance_cycle_phase(uint64_t delivered_mstamp);
        
        /* Start a new long-term sampling interval. */
        void bbr_reset_lt_bw_sampling_interval(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost);

        /* Completely reset long-term bandwidth sampling. */
        void bbr_reset_lt_bw_sampling(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost);

        /* Long-term bw sampling interval is done. Estimate whether we're policed. */
        void bbr_lt_bw_interval_done(uint64_t bw, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost);

        /* Token-bucket traffic policers are common (see "An Internet-Wide Analysis of
        * Traffic Policing", SIGCOMM 2016). BBR detects token-bucket policers and
        * explicitly models their policed rate, to reduce unnecessary losses. We
        * estimate that we're policed if we see 2 consecutive sampling intervals with
        * consistent throughput and high packet loss. If we think we're being policed,
        * set lt_bw to the "long-term" average delivery rate from those 2 intervals.
        */
        // ��������������������������������䲢�д�������������Ϊ��traffic policers��������������long-term״̬�������һ����������
        void bbr_lt_bw_sampling(uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool is_app_limited);

        /* Estimate the bandwidth based on how fast packets are delivered */
        void bbr_update_bw(uint32_t rrt, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool app_limit);

        /* Estimate when the pipe is full, using the change in delivery rate: BBR
         * estimates that STARTUP filled the pipe if the estimated bw hasn't changed by
         * at least bbr_full_bw_thresh (25%) after bbr_full_bw_cnt (3) non-app-limited
         * rounds. Why 3 rounds: 1: rwin autotuning grows the rwin, 2: we fill the
         * higher rwin, 3: we get higher delivery rate samples. Or transient
         * cross-traffic or radio noise can go away. CUBIC Hystart shares a similar
         * design goal, but uses delay and inter-ACK spacing instead of bandwidth.
         */
         /*
           ��������׶δ����Ƿ��Ѿ��������ֵ��
           ����������μ�鵽���������ٶ�С��bbr_full_bw_thresh(25%),
           ����Ϊpipe����,�����������ֵ
         */
        void bbr_check_full_bw_reached(bool app_limite);

        /* If pipe is probably full, drain the queue and then enter steady-state. */
        // ���pipe�Ѿ��������С�����ٶ�
        // ���STARTUP״̬������������л���DRAIN״̬�����DRAIN�׶� ������գ��л���BBR_PROBE_BW״̬
        void bbr_check_drain(uint64_t delivered_mstamp, uint32_t pacing_rate, uint32_t inflight);

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
        // ����Ƿ�ý���rtt̽��״̬�Լ���Ӧ��������
        void bbr_update_min_rtt(uint64_t delivered_mstamp, uint32_t inflight, uint64_t delivered, uint32_t rtt_us, uint32_t& send_wnd);
        void bbr_init();
        void bbr_update_model(uint32_t pacing_rate, uint32_t inflight, uint32_t rrt,
            uint64_t delivered_mstamp, uint32_t delivered, 
            uint32_t lost, bool app_limit,
            uint32_t& send_wnd);

    public:
        CBbr() { bbr_init(); }
        ~CBbr() {}
        // BBR driven by incoming parameters, get send_wnd and pacint_rate.
        // params:
        // inflight : ���ܱ�ackʱ�����е�������, ������֮��δȷ�ϵ�������
        // rtt      : ����۲��rtt
        // acked    : ����ackȷ�ϵİ���, ack��ϲ�����
        // delivered: ����ackȷ�ϵ�������
        // delivered_mstamp: ����ackȷ��ʱ���
        // lost     : �ڼ䶪������
        // app_limit: �Ƿ�Ӧ�ò�����
        // send_wnd     �����ʹ����С
        // pacing_rate  ����������
        void bbr_main(uint32_t inflight, uint32_t rrt, uint32_t acked,
                      uint64_t delivered_mstamp, uint32_t delivered,
                      uint32_t lost, bool app_limit,
                      uint32_t& send_wnd, uint32_t& pacing_rate);

    };
}

#endif