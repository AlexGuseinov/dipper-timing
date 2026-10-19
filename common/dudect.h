/* dudect-style fixed-vs-random timing test, platform-independent part.
 *
 * Each platform provides "platform.h" (on the include path) with:
 *   typedef ... plat_t;                       raw timer value
 *   static inline plat_t   plat_now(void);    read the timer (serialising)
 *   static inline uint32_t plat_delta(plat_t t0, plat_t t1);  elapsed cycles/ticks
 *   static inline void     plat_irq_off(void), plat_irq_on(void);
 *   void plat_print(const char *line);        print one line + "\r\n", flushed
 *   #define PLAT_NAME "..."  PLAT_TIMER "..."
 *
 * Statistics follow dudect (Reparaz, Balasch, Verbauwhede, DATE 2017,
 * reference code oreparaz/dudect): Welford running mean/variance per class,
 * Welch t; test 0 = no crop, tests 1..NCROP = samples below percentile
 * p_k = 1 - 0.5^k (k = 1..NCROP, a subset of dudect's 100 levels), thresholds
 * from a calibration batch that is then discarded; test NCROP+1 = second order
 * (centred square), started once test 0 has more than DUDECT_ENOUGH samples.
 * Reported value: max |t| over tests with more than DUDECT_ENOUGH samples. */
#ifndef DUDECT_H
#define DUDECT_H
#include <stdint.h>

#ifndef DUDECT_NCROP
#define DUDECT_NCROP 10
#endif
#define DUDECT_NTESTS (DUDECT_NCROP + 2)
#ifndef DUDECT_ENOUGH
#define DUDECT_ENOUGH 10000
#endif

typedef struct { double mean[2], m2[2], n[2]; } welch_t;

typedef struct {
    welch_t  t[DUDECT_NTESTS];
    uint32_t thr[DUDECT_NCROP];   /* crop thresholds from calibration */
    uint32_t xmin[2], xmax[2];    /* per-class range of all measurements */
} dudect_state_t;

void   dudect_init(dudect_state_t *s, uint32_t *cal, uint32_t ncal); /* sorts cal */
void   dudect_push(dudect_state_t *s, uint32_t x, int cls);
double dudect_welch(const welch_t *w);
/* max |t| over tests with enough samples; *which = test index (-1 if none) */
double dudect_max_t(const dudect_state_t *s, int *which);

/* ---- the experiment driver ---- */
enum { TEST_A = 0, TEST_B = 1 };   /* A: fixed vs random plaintext; B: fixed vs random key */

typedef struct {
    uint32_t seed;        /* PRNG seed for class labels and random inputs */
    uint32_t n;           /* measurements per (variant, test), after calibration */
    uint32_t ncal;        /* calibration measurements */
    uint32_t nraw;        /* raw samples dumped per (variant, test) */
    uint32_t variants;    /* bit mask over dipper_variants[] */
    uint32_t tests;       /* bit 0 = Test A, bit 1 = Test B */
} dudect_cfg_t;

/* Runs selftest, cost measurement and all selected (variant, test) pairs,
 * printing the protocol described in docs/PROTOCOL.md. Returns 0 if ok. */
int dudect_run_all(const dudect_cfg_t *cfg);
#endif
