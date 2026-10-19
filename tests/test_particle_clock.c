#include <assert.h>
#include <stdio.h>
#include "particle_clock.h"

int main(void) {
    for (unsigned initial = 0; initial < RA3_FRAMES_PER_PARTICLE_STEP; ++initial) {
        unsigned phase = initial, updates = 0;
        unsigned prior = 0, saw_prior = 0;
        for (unsigned frame = 0; frame < RA3_TARGET_FPS * 1000u; ++frame) {
            if (particle_clock_step(&phase)) {
                if (saw_prior) assert(frame - prior == RA3_FRAMES_PER_PARTICLE_STEP);
                prior = frame;
                saw_prior = 1;
                ++updates;
            }
            assert(phase < RA3_FRAMES_PER_PARTICLE_STEP);
            if ((frame + 1) % RA3_TARGET_FPS == 0)
                assert(updates == (frame + 1) / RA3_FRAMES_PER_PARTICLE_STEP);
        }
        assert(phase == initial && updates == 30000);
    }
    printf("PASS: 30 particle steps per%uclientframes; all %u phases, no burst or drift.\n", RA3_TARGET_FPS, RA3_FRAMES_PER_PARTICLE_STEP);
    return 0;
}
