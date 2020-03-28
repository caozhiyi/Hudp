#ifndef HEADER_NET_CONTROLLER_OSCOMMON
#define HEADER_NET_CONTROLLER_OSCOMMON

#include "CommonType.h"

namespace hudp {

#define USEC_PER_MSEC	1000L
#define USEC_PER_SEC	1000000L
    // about minmax
    /* A single data point for our parameterized min-max tracker */
    struct minmax_sample {
        uint32_t	t;	/* time measurement was taken */
        uint32_t	v;	/* value measured */
    };

    /* State for the parameterized min-max tracker */
    struct minmax {
        struct minmax_sample s[3];
    };

    static inline uint32_t minmax_get(const struct minmax *m) {
        return m->s[0].v;
    }

    static inline uint32_t minmax_reset(struct minmax *m, uint32_t t, uint32_t meas)
    {
        struct minmax_sample val = { t, meas };

        m->s[2] = m->s[1] = m->s[0] = val;
        return m->s[0].v;
    }

    /* As time advances, update the 1st, 2nd, and 3rd choices. */
    static uint32_t minmax_subwin_update(struct minmax *m, uint32_t win,
        const struct minmax_sample *val)
    {
        uint32_t dt = val->t - m->s[0].t;

        if ((dt > win)) {
            /*
             * Passed entire window without a new val so make 2nd
             * choice the new val & 3rd choice the new 2nd choice.
             * we may have to iterate this since our 2nd choice
             * may also be outside the window (we checked on entry
             * that the third choice was in the window).
             */
            m->s[0] = m->s[1];
            m->s[1] = m->s[2];
            m->s[2] = *val;
            if ((val->t - m->s[0].t > win)) {
                m->s[0] = m->s[1];
                m->s[1] = m->s[2];
                m->s[2] = *val;
            }
        }
        else if ((m->s[1].t == m->s[0].t) && dt > win / 4) {
            /*
             * We've passed a quarter of the window without a new val
             * so take a 2nd choice from the 2nd quarter of the window.
             */
            m->s[2] = m->s[1] = *val;
        }
        else if ((m->s[2].t == m->s[1].t) && dt > win / 2) {
            /*
             * We've passed half the window without finding a new val
             * so take a 3rd choice from the last half of the window
             */
            m->s[2] = *val;
        }
        return m->s[0].v;
    }

    /* Check if new measurement updates the 1st, 2nd or 3rd choice max. */
    static uint32_t minmax_running_max(struct minmax *m, uint32_t win, uint32_t t, uint32_t meas)
    {
        struct minmax_sample val = { t, meas };

        if ((val.v >= m->s[0].v) ||	  /* found new max? */
            (val.t - m->s[2].t > win))	  /* nothing left in window? */
            return minmax_reset(m, t, meas);  /* forget earlier samples */

        if ((val.v >= m->s[1].v))
            m->s[2] = m->s[1] = val;
        else if ((val.v >= m->s[2].v))
            m->s[2] = val;

        return minmax_subwin_update(m, win, &val);
    }

}

#endif