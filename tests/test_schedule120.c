#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "schedule120.h"

static unsigned expected_end(unsigned slot) {
#if RA3_TARGET_FPS == 120
    /* Preserve the visually accepted schedule: holds at 3/5, stage6 unique. */
    static const unsigned accepted[8] = {1, 2, 3, 3, 4, 5, 5, 6};
    return accepted[slot - 1];
#else
    return (6 * slot + RA3_DISPLAY_RATIO - 1) / RA3_DISPLAY_RATIO;
#endif
}

int main(void) {
    unsigned remainder = 0, milliseconds = 0;
    unsigned native_stages = 0, cycles = 0, holds = 0;
#if RA3_CORE_HOLD_SCHEDULER
    Schedule120 schedule = {0};
#endif
    unsigned stage = 6;
    for (unsigned frame = 1; frame <= RA3_TARGET_FPS * 1000u; ++frame) {
        unsigned slot = (frame - 1) % RA3_DISPLAY_RATIO + 1;
        unsigned first = stage == 6 ? 1 : stage + 1;
#if RA3_CORE_HOLD_SCHEDULER
        unsigned hold = schedule120_begin(&schedule, stage);
        if (hold) {
            assert(RA3_TARGET_FPS == 120 && (stage == 3 || stage == 5));
            holds++;
        } else {
            stage = first;
            native_stages++;
            if (stage == 1) cycles++;
        }
        assert(schedule.slot == slot);
        assert(stage == schedule120_end(slot));
        unsigned bits = interpolation_bits(slot);
#elif RA3_TARGET_FPS == 60
        stage = end_stage(first);
        native_stages += stage - first + 1;
        if (first == 1) cycles++;
        unsigned bits = interpolation_bits(stage);
#else
        /* Native 30 FPS batches three of the six stages per display. */
        stage = expected_end(slot);
        native_stages += stage - first + 1;
        if (first == 1) cycles++;
        (void)end_stage;
        (void)interpolation_bits;
        unsigned bits;
        float native_alpha = (float)slot / RA3_DISPLAY_RATIO;
        memcpy(&bits, &native_alpha, 4);
#endif
        assert(stage == expected_end(slot));
        float alpha;
        memcpy(&alpha, &bits, 4);
        assert(alpha == (float)slot / RA3_DISPLAY_RATIO);
        unsigned delta = next_milliseconds(&remainder);
        assert(delta == 1000u / RA3_TARGET_FPS || delta == 1000u / RA3_TARGET_FPS + 1);
        milliseconds += delta;
        assert(milliseconds == (uint64_t)frame * 1000 / RA3_TARGET_FPS);
        if (frame % RA3_TARGET_FPS == 0) {
            assert(native_stages == frame / RA3_TARGET_FPS * 90);
            assert(cycles == frame / RA3_TARGET_FPS * 15);
        }
    }
#if RA3_CORE_HOLD_SCHEDULER
    assert(!schedule120_begin(&schedule, 99));
    assert(!schedule.synchronized && schedule.slot == 0);
    assert(!schedule120_begin(&schedule, 6));
    assert(schedule.synchronized && schedule.slot == 1);
#else
    assert(holds == 0);
#endif
    printf("PASS: %u FPS, six native stages per15Hzcycle, exact rational clock, %u hold slots over1000seconds.\n", RA3_TARGET_FPS, holds);
    return 0;
}
