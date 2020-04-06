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
    return (uint32_t)rate;
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
    // ����bbr �� BBR_STARTUP ģʽ �����ٶ��������Ӻʹ�������������Ϊ����ֵ
    mode = BBR_STARTUP;
    pacing_gain = bbr_high_gain;
    cwnd_gain = bbr_high_gain;
}

void CBbr::bbr_reset_probe_bw_mode(uint64_t delivered_mstamp) {
    // ����bbr �� BBR_PROBE_BW ģʽ����С ��������
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
    // ��ʼ�׶�δ�ﵽ������
    if (!bbr_full_bw_reached())
        // ����Ϊ BBR_STARTUP
        bbr_reset_startup_mode();
    else
        // ����Ϊ BBR_PROBE_BW
        bbr_reset_probe_bw_mode(delivered_mstamp);
}

uint32_t CBbr::bbr_min_tso_segs(uint32_t pacing_rate) {
    return pacing_rate < (bbr_min_tso_rate >> 3) ? 1 : 2;
}

void CBbr::bbr_save_cwnd(uint32_t send_wnd) {
    if (mode != BBR_PROBE_RTT)
        prior_cwnd = send_wnd;  /* this cwnd is good enough */
    else  /* loss recovery or BBR_PROBE_RTT have temporarily cut cwnd */
        prior_cwnd = prior_cwnd > send_wnd ? prior_cwnd : send_wnd;
}

void CBbr::bbr_update_cycle_phase(uint32_t pacing_rate, uint64_t delivered_mstamp, uint32_t inflight, bool loss_pkt) {
    // ����BBR_PROBE_BW �׶Σ� 
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
    uint64_t cwnd;
    uint64_t w;
    /* If we've never had a valid RTT sample, cap cwnd at the initial
        * default. This should only happen when the connection is not using TCP
        * timestamps and has retransmitted all of the SYN/SYNACK/data packets
        * ACKed so far. In this case, an RTO can cut cwnd to 1, in which
        * case we need to slow-start up toward something safe: TCP_INIT_CWND.
        */
        // û����Ч��rtt�� ���ͳ�ʼ�����С
    if (min_rtt_us == ~0U)	    /* no valid RTT samples yet? */
        return TCP_INIT_CWND;   /* be safe: cap at default initial cwnd*/

    w = (uint64_t)bw * min_rtt_us;

    /* Apply a gain to the given value, then remove the BW_SCALE shift. */
    cwnd = (((w * gain) >> BBR_SCALE) + BW_UNIT - 1) / BW_UNIT;

    /* Allow enough full-sized skbs in flight to utilize end systems. */
    cwnd += 3 * bbr_tso_segs_goal(pacing_rate);

    /* Reduce delayed ACKs by rounding up cwnd to the next even number. */
    // ͨ����cwnd���뵽��һ��ż���������ӳ�ack
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
        ���delivered_mstamp-cycle_mstamp>min_rtt_us��
        ����һ�ִ���̽��ʱ������.
        ���is_full_length=true������is_full_length=false
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
        �������̽��������������(pacing_gain=5/4)
        ���û�ж�����
        ����һֱ���ڸ�����ֱ���������ӵ�Ŀ�괰��(pacing_gain*BDP)
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
            ��������ſն�������(pacing_gain=3/4),̽��ʱ�����˻���
            inflightС��Ŀ�괰�ڿ�����ǰ�˳��������
        */
    return is_full_length ||
        inflight <= bbr_target_cwnd(pacing_rate, bw, BBR_UNIT);
}

void CBbr::bbr_advance_cycle_phase(uint64_t delivered_mstamp) {
    cycle_idx = (cycle_idx + 1) & (CYCLE_LEN - 1);
    cycle_mstamp = delivered_mstamp;
    // ���������������
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

void CBbr::bbr_lt_bw_interval_done(uint64_t bw, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost) {
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
    uint64_t t;

    /*
    ����Ѿ�����longterm״̬
    �������bbr����̽��׶Σ��ҽ��������ڣ�longtermʱ���Ѿ�������48�����ڣ�������lt�������л���bbr����̽��׶�
    �˳�longterm״̬
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
        ���lt_is_sampling==fasle,��û�в���
        ���rs->losses==0,��û�ж��������˳�long-term���
        ��ʼһ���µ�long-term���������²�����ǩ
        */
    if (!lt_is_sampling) {
        if (!lost)
            return;
        bbr_reset_lt_bw_sampling_interval(delivered_mstamp, delivered, lost);
        lt_is_sampling = true;
    }

    /* To avoid underestimates, reset sampling if we run out of data. */
    /* �����Ӧ�ò��������ƣ������²��� */
    if (is_app_limited) {
        bbr_reset_lt_bw_sampling(delivered_mstamp, delivered, lost);
        return;
    }

    /* ������������Ҫ��(4,16)֮�� */
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
        /* ���rs->losses==0,��û�ж��������˳�long-term��� */
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

void CBbr::bbr_update_bw(uint32_t rrt, uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool app_limit) {
    uint32_t cur_bw;
    round_start = 0;

    // ���βɼ�������Ч
    if (delivered < 0 || rrt <= 0)
        return; /* Not a valid observation */

    /* See if we've reached the next RTT */
    // �������Ƿ񵽴���һ��RTT
    /* rs->prior_deliveredΪ���ڿ�ʼ����tp->delivered, bbr->next_rtt_deliveredΪ��һ�����ڵ�tp->delivered����
    ���rs->priorr_delivered>=bbr->next_rtt_deliverd����������һ�����ڣ�
        ���������µ���һ�����ڵĿ�ʼ���Ĵ�������next_rtt_delivered������rtt_cnt*/
    if (rtt_cnt > 1000/*TODO*/) {
        next_rtt_delivered = delivered;
        rtt_cnt++;
        round_start = 1;
    }

    bbr_lt_bw_sampling(delivered_mstamp, delivered, lost, app_limit);

    /* Divide delivered by the interval to find a (lower bound) bottleneck
        * bandwidth sample. Delivered is in packets and interval_us in uS and
        * ratio will be <<1 for most connections. So delivered is first scaled.
        */
        // ����bw ��������/ʱ��
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
        // ��app�����ұ�֮ǰ�۲⵽��bw���󣬸���bw
    if (!app_limit || cur_bw >= bbr_max_bw()) {
        /* Incorporate new sample into our max bw filter. */
        minmax_running_max(&bw, bbr_bw_rtts, rtt_cnt, cur_bw);
    }
}

void CBbr::bbr_check_full_bw_reached(bool app_limite) {
    uint32_t bw_thresh;

    /*
        ����Ѿ�����˴������ˣ�����round_start=0,����δ��ʼ�⣬����Ӧ�ò�����ס�ˣ�ֱ���˳�
    */
    if (bbr_full_bw_reached() || !round_start || app_limite)
        return;

    // bwû������ bbr_full_bw_thresh 25%
    /*
        ���bbr���ڵ���������������������趨����ֵbbr_full_bw_thresh��
        �����������bbr->full_bw������full_bw_cnt��0
        ����full_bw_cnt+1��full_bw_cnt����3�ͱ�ʾ��������
    */
    bw_thresh = (uint64_t)full_bw * bbr_full_bw_thresh >> BBR_SCALE;
    if (bbr_max_bw() >= bw_thresh) {
        full_bw = bbr_max_bw();
        full_bw_cnt = 0;
        return;
    }
    // ����Ϊpipe�Ѿ���
    ++full_bw_cnt;
    full_bw_reached = full_bw_cnt >= bbr_full_bw_cnt;
}

void CBbr::bbr_check_drain(uint64_t delivered_mstamp, uint32_t pacing_rate, uint32_t inflight) {
    // �����BBR_STARTUP�׶�pipe�����������BBR_DRAIN�׶�
    /*
        ����������׶δ�������(���ڲ��䣬�ٶȼ�С)
        ����״̬�����ſ�״̬
        ����pacing_gain
        ����cwnd_gain
    */
    if (mode == BBR_STARTUP && bbr_full_bw_reached()) {
        mode = BBR_DRAIN;	/* drain queue we created */
        pacing_gain = bbr_drain_gain;	/* pace slow to drain �����ļ�С*/
        cwnd_gain = bbr_high_gain;	/* maintain cwnd ���ַ��ʹ����С*/
    }	/* fall through to check if in-flight is already small: */

    // �����BBR_DRAIN�׶�
    /*
        ����ſս׶�inflight<=target���������Ѿ���գ��л�������̽��״̬��
    */
    if (mode == BBR_DRAIN && inflight <= bbr_target_cwnd(pacing_rate, bbr_max_bw(), BBR_UNIT))
        bbr_reset_probe_bw_mode(delivered_mstamp);  /* we estimate queue is drained ���ǹ��ƶ����Ѻľ�*/
}

void CBbr::bbr_check_probe_rtt_done(uint64_t delivered_mstamp, uint32_t& send_wnd) {
    // probe_rtt_done_stampδ����
    if (!(probe_rtt_done_stamp &&
        GetCurTimeStamp() < probe_rtt_done_stamp))
        return;

    // �ȴ�һ��ʱ�䣬ֱ��̽�����
    min_rtt_stamp = GetCurTimeStamp();  /* wait a while until PROBE_RTT */
    // ���÷��ʹ����С
    send_wnd = std::max(send_wnd, prior_cwnd);
    bbr_reset_mode(delivered_mstamp);
}

void CBbr::bbr_update_min_rtt(uint64_t delivered_mstamp, uint32_t inflight, uint64_t delivered, uint32_t rtt_us, uint32_t& send_wnd) {
    bool filter_expired;

    /* Track min RTT seen in the min_rtt_win_sec filter window: */
    // ��ǰ����Сrtt�ɼ�ʱ����Ƿ��Ѿ����� 10s ��ʱ�䴰��
    /*
        ��Сrtt��Чʱ��Ϊbbr_min_rtt_win_sec*HZ, ��10s, �����Чʱ����˻����²ɵ�rtt��С��
        ������Сrtt��С����Сrtt������ʱ��
    */
    filter_expired = GetCurTimeStamp() < min_rtt_stamp + bbr_min_rtt_win_sec * HZ;
    // ���βɼ���rttʱ�䣬 �ұȵ�ǰ����СrttС������ǰ��Сrtt�ɼ�ʱ����Ѿ����ڣ�����ack�����ӳ�ȷ�ϵ�ack
    if (rtt_us >= 0 && (rtt_us <= min_rtt_us || (filter_expired))) {
        min_rtt_us = rtt_us;
        min_rtt_stamp = GetCurTimeStamp();
    }

    // filter_expired�Ѿ���ȥ
    /*
        ���������rtt̽�⹦�ܣ�����Сrtt��Чʱ�����(Ҳ��������Ϊrtt̽�����ڵ���)��
        ��idle_restart==0�����Ǵӿ���״̬�����ģ����ҵ�ǰ������rtt̽��״̬BBR_PROBE_RTT��
        ����״̬��ΪBBR_PROBE_RTT����С�����ٶȺͷ��ʹ��ڣ�����֮ǰ���������ָ���
        rtt̽�����ʱ����Ϊ��Чֵ0����������þ�����Чֵ��
    */
    if (bbr_probe_rtt_mode_ms > 0 && filter_expired && !idle_restart && mode != BBR_PROBE_RTT) {
        // ����BBR_PROBE_RTTģʽ
        mode = BBR_PROBE_RTT;  /* dip, drain queue */
        pacing_gain = BBR_UNIT;
        cwnd_gain = BBR_UNIT;
        bbr_save_cwnd(send_wnd);  /* note cwnd so we can restore it */
        probe_rtt_done_stamp = 0;
    }
    /*
    �������rtt̽��״̬����������
    ���probe_rtt_done_stamp=0�����������Ч�����������еİ�����bbr_cwnd_min_target
        ����rtt̽�����ʱ��,����probe_rtt_round_done=0(���rtt̽�⻹û�п�ʼ����),������һ��rtt��delivered
    �������probe_rtt_done_stamp!=0�����������Ч��
        ���round_start=1
            ���probe_rtt_round_done=1��rtt̽���Ѿ���ʼ���ˣ�
        ���rtt̽���Ѿ��������ˣ�����̽��ʱ������
            ������Сrtt�����ʱ�䣨�����ж���û�й��ڣ������½���rtt̽�����ڣ�
            ��ǻָ�����
            ����ģ��
    */
    if (mode == BBR_PROBE_RTT) {
        /* Maintain min packets in flight for max(200 ms, 1 round). */
        // �ڷ����б���������ݰ���200���룬1�֣�
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

void CBbr::bbr_init() {
    prior_cwnd = 0;
    rtt_cnt = 0;
    next_rtt_delivered = 0;

    probe_rtt_done_stamp = 0;
    probe_rtt_round_done = 0;
    min_rtt_us = 0;
    min_rtt_stamp = GetCurTimeStamp();

    minmax_reset(&bw, rtt_cnt, 0);  /* init max bw to 0 */

    has_seen_rtt = 0;

    round_start = 0;
    idle_restart = 1;
    full_bw_reached = 0;
    full_bw = 0;
    full_bw_cnt = 0;
    cycle_mstamp = 0;
    cycle_idx = 0;
    bbr_reset_startup_mode();
}

void CBbr::bbr_update_model(uint32_t pacing_rate, uint32_t inflight, uint32_t rrt,
    uint64_t delivered_mstamp,  uint32_t delivered, 
    uint32_t lost, bool app_limit,
    uint32_t& send_wnd) {
    //���µ�ǰ���bw
    bbr_update_bw(rrt, delivered_mstamp, delivered, lost, app_limit);
    // ������������
    bbr_update_cycle_phase(pacing_rate, delivered_mstamp, inflight, lost > 0);
    // ���pipe�Ѿ���
    bbr_check_full_bw_reached(app_limit);
    // ��������Ѿ����� ����Ƿ�Ӧ�ü�С�����ٶ�
    bbr_check_drain(delivered_mstamp, pacing_rate, inflight);
    // ������Сrttʱ��
    bbr_update_min_rtt(delivered_mstamp, inflight, delivered, rrt, send_wnd);
}

void CBbr::bbr_main(uint32_t inflight, uint32_t rrt, uint32_t acked,
    uint64_t delivered_mstamp, uint32_t delivered, uint32_t lost, bool app_limit, 
    uint32_t& send_wnd, uint32_t& pacing_rate) {

    uint32_t bw;
    bbr_update_model(pacing_rate, inflight, rrt,
        delivered_mstamp, delivered, lost, app_limit,
        send_wnd);

    bw = bbr_bw();
    // ���÷����ٶȺʹ����С
    bbr_set_pacing_rate(rrt, send_wnd, bw, pacing_gain, pacing_rate);
    bbr_set_cwnd(pacing_rate, acked, bw, pacing_gain, send_wnd);
}