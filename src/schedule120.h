#ifndef RA3_SCHEDULE120_H
#define RA3_SCHEDULE120_H

#include "rate_config.h"

/* Fixed display/logic schedule. The native logic clock stays 15 Hz; the
 * display side is configured by RA3_TARGET_FPS. Two scheduler families:
 *   - ratio >= 6 (90/120): the native high-ratio branch advances at most one
 *     native stage per display; this module decides which slots hold. The
 *     ratio>=6 native path itself is never altered.
 *   - ratio 4 (60): the native batcher advances 1-2 stages per display via
 *     the accepted 0x621ace end-stage hook; endpoints stay 2/3/5/6.
 * ratio 2 (30) is native engine behavior; only the display clock runs. */

typedef struct {
    unsigned slot;
    unsigned synchronized;
} Schedule120;

#if RA3_CORE_HOLD_SCHEDULER

static unsigned schedule120_end(unsigned slot) {
#if RA3_DISPLAY_RATIO == 8
    static const unsigned ends[8] = {1, 2, 3, 3, 4, 5, 5, 6};
#else
    static const unsigned ends[6] = {1, 2, 3, 4, 5, 6};
#endif
    return ends[(slot - 1) % RA3_DISPLAY_RATIO];
}

/* Called with the stage before native advancement. Nonzero means hold. */
static unsigned schedule120_begin(Schedule120 *schedule, unsigned stage) {
    if (stage > 6) {
        schedule->slot = 0;
        schedule->synchronized = 0;
        return 0;
    }
    if (schedule->synchronized && schedule->slot &&
        stage != schedule120_end(schedule->slot)) {
        schedule->slot = 0;
        schedule->synchronized = 0;
    }
    if (!schedule->synchronized) {
        /* Finish an in-flight native cycle before establishing slot one. */
        if (stage != 6 && stage != 0) return 0;
        schedule->slot = 0;
        schedule->synchronized = 1;
    }
    schedule->slot = (schedule->slot % RA3_DISPLAY_RATIO) + 1;
    return stage == schedule120_end(schedule->slot);
}

/* Interpolation fraction per display slot: slot/ratio of the logic cycle
 * (exact eighths at 120, exact sixths at 90). */
static unsigned interpolation_bits(unsigned slot) {
#if RA3_DISPLAY_RATIO == 8
    static const unsigned bits[8] = {
        0x3e000000, 0x3e800000, 0x3ec00000, 0x3f000000,
        0x3f200000, 0x3f400000, 0x3f600000, 0x3f800000
    };
#else
    static const unsigned bits[6] = {
        0x3e2aaaab, 0x3eaaaaab, 0x3f000000, 0x3f2aaaab, 0x3f555555, 0x3f800000
    };
#endif
    return bits[(slot - 1) % RA3_DISPLAY_RATIO];
}

#else /* ratio 4: accepted native batching; ratio 2: native clock only */

/* Native display endpoints per batched first stage; display one always
 * covers stages 1-2 through the FIRST_DISPLAY threshold 6 -> 8. */
static inline unsigned end_stage(unsigned first) {
    if (first <= 2) return 2;
    if (first == 3) return 3;
    if (first <= 5) return 5;
    return 6;
}

static unsigned interpolation_bits(unsigned stage) {
    if (stage <= 2) return 0x3e800000; /* 1/4 */
    if (stage == 3) return 0x3f000000; /* 2/4 */
    if (stage <= 5) return 0x3f400000; /* 3/4 */
    return 0x3f800000;                 /* 4/4 */
}

#endif /* scheduler family */

/* Bresenham display milliseconds: adds exactly 1000 ms per RA3_TARGET_FPS
 * displays (8/9 at 120, 11/12 at 90, 16/17 at 60, 33/34 at 30). */
static unsigned next_milliseconds(unsigned *remainder) {
    *remainder += 1000;
    unsigned milliseconds = *remainder / RA3_TARGET_FPS;
    *remainder %= RA3_TARGET_FPS;
    return milliseconds;
}
#endif
