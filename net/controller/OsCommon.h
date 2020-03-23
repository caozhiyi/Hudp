#ifndef HEADER_NET_CONTROLLER_OSCOMMON
#define HEADER_NET_CONTROLLER_OSCOMMON

#include "CommonType.h"

namespace hudp {

#define USEC_PER_MSEC	1000L
#define USEC_PER_SEC	1000000L

    /* The unnecessary pointer compare is there
     * to check for type safety (n must be 64bit)
     */
# define do_div(n,base) ({				\
	uint32_t __base = (base);			\
	uint32_t __rem;					\
	(void)(((typeof((n)) *)0) == ((uint64_t *)0));	\
	if (__builtin_constant_p(__base) &&		\
	    is_power_of_2(__base)) {			\
		__rem = (n) & (__base - 1);		\
		(n) >>= ilog2(__base);			\
	} else if (__div64_const32_is_OK &&		\
		   __builtin_constant_p(__base) &&	\
		   __base != 0) {			\
		uint32_t __res_lo, __n_lo = (n);	\
		(n) = __div64_const32(n, __base);	\
		/* the remainder can be computed with 32-bit regs */ \
		__res_lo = (n);				\
		__rem = __n_lo - __res_lo * __base;	\
	} else if (likely(((n) >> 32) == 0)) {		\
		__rem = (uint32_t)(n) % __base;		\
		(n) = (uint32_t)(n) / __base;		\
	} else 						\
		__rem = __div64_32(&(n), __base);	\
	__rem;						\
 })

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

}

#endif