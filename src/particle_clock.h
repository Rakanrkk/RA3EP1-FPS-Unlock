#ifndef RA3_PARTICLE_CLOCK_H
#define RA3_PARTICLE_CLOCK_H

#include "rate_config.h"

/* Particle emitters authored for 30 Hz advance once per
 * RA3_FRAMES_PER_PARTICLE_STEP client frames (4 at 120, 3 at 90, 2 at 60,
 * 1 at 30). The renderer still runs every frame; no wall-clock thread is
 * added. The first client frame of each cycle steps, preserving the accepted
 * 120 Hz first-frame choice (phase 0 runs). */
static unsigned particle_clock_step(unsigned *phase) {
    unsigned run = *phase == 0;
#if RA3_FRAMES_PER_PARTICLE_STEP > 1
    *phase = (*phase + 1) % RA3_FRAMES_PER_PARTICLE_STEP;
#endif
    return run;
}
#endif
