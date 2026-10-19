#ifndef RA3_RATE_CONFIG_H
#define RA3_RATE_CONFIG_H

/* Compile-time display-rate selection for the generalized Uprising timing
 * payload. One source tree builds one DLL per target rate; there are no
 * runtime rate switches and no source-file copies. The native engine logic
 * clock stays fixed at 15 Hz ("original logic 15"); only display-side
 * constants derived below change.
 *
 * RA3_TARGET_FPS selects the display/client update rate. Anything other than
 * 30, 60, 90 or 120 is rejected statically at compile time. The default is
 * 120, which must keep the accepted 120 FPS schedule semantics exactly.
 *
 * Exposed rate constants (also consumed by the host tests and by main's
 * separate i686 ABI runtime test):
 *   RA3_TARGET_FPS                display/client updates per second
 *   RA3_DISPLAY_RATIO             display slots per 15 Hz logic cycle
 *   RA3_FRAMES_PER_PARTICLE_STEP  client frames per authored 30 Hz particle
 *                                 simulation step (frames per physics)
 *   RA3_SWAY_STEPS                display steps per authored 30 Hz sway cycle
 *   RA3_UNIT_WINDOW               drawable interpolation age window (frames)
 *   RA3_CORE_HOLD_SCHEDULER       1 when the ratio>=6 hold scheduler is used
 *   RA3_CORE_DISABLED             1 when core timing edits are off (30 Hz)
 *   RA3_EXPECTED_CORE_EDITS       exact core edit count committed by install
 */

#ifndef RA3_TARGET_FPS
#define RA3_TARGET_FPS 120
#endif

#if RA3_TARGET_FPS == 120
/* Accepted schedule: 8 slots / 6 stages, holds on stages 3 and 5. */
#define RA3_DISPLAY_RATIO 8
#define RA3_FRAMES_PER_PARTICLE_STEP 4
#define RA3_SWAY_STEPS 4
#define RA3_UNIT_WINDOW 10 /* three age guards widened 6 -> 10 */
#define RA3_CORE_HOLD_SCHEDULER 1
#define RA3_CORE_DISABLED 0
#define RA3_EXPECTED_CORE_EDITS 30
#elif RA3_TARGET_FPS == 90
/* Six slots, one native stage per display on the native ratio>=6 path. */
#define RA3_DISPLAY_RATIO 6
#define RA3_FRAMES_PER_PARTICLE_STEP 3
#define RA3_SWAY_STEPS 3
#define RA3_UNIT_WINDOW 8 /* six-frame cycle plus two frames of eligibility margin */
#define RA3_CORE_HOLD_SCHEDULER 1
#define RA3_CORE_DISABLED 0
#define RA3_EXPECTED_CORE_EDITS 30
#elif RA3_TARGET_FPS == 60
/* Native 4:1 batching (endpoints 2/3/5/6) as accepted in Uprising60-v0.12,
 * plus the corrected 30 Hz particle throttle and sway interpolation. */
#define RA3_DISPLAY_RATIO 4
#define RA3_FRAMES_PER_PARTICLE_STEP 2
#define RA3_SWAY_STEPS 2
#define RA3_UNIT_WINDOW 6 /* native window spans the 4-frame cycle */
#define RA3_CORE_HOLD_SCHEDULER 0
#define RA3_CORE_DISABLED 0
#define RA3_EXPECTED_CORE_EDITS 29
#elif RA3_TARGET_FPS == 30
/* Native engine timing; counter-only DLL, no core edits, no fake 120. */
#define RA3_DISPLAY_RATIO 2
#define RA3_FRAMES_PER_PARTICLE_STEP 1
#define RA3_SWAY_STEPS 1
#define RA3_UNIT_WINDOW 6
#define RA3_CORE_HOLD_SCHEDULER 0
#define RA3_CORE_DISABLED 1
#define RA3_EXPECTED_CORE_EDITS 0
#else
#error "RA3_TARGET_FPS must be 30, 60, 90, or 120"
#endif

/* A bounded 90 FPS comparison of the three drawable age guards. */
#ifdef RA3_DIAGNOSTIC_UNIT_WINDOW
#if RA3_TARGET_FPS != 90 || RA3_DIAGNOSTIC_UNIT_WINDOW != 10
#error "The diagnostic age-window override is limited to 90 FPS / 10 frames"
#endif
#undef RA3_UNIT_WINDOW
#define RA3_UNIT_WINDOW RA3_DIAGNOSTIC_UNIT_WINDOW
#endif

#define RA3_LOGIC_HZ 15
#define RA3_NOMINAL_FRAME_MS (1000 / RA3_TARGET_FPS)
_Static_assert(RA3_TARGET_FPS == RA3_LOGIC_HZ * RA3_DISPLAY_RATIO,
               "display ratio must divide the target rate exactly");

/* GPU particle resources keep authored 30 Hz lifetime units at every rate;
 * the client-domain scale that replaces the authored 0.03 constant scales
 * with the display rate (written as explicit float literals so the accepted
 * 120 Hz value stays byte-identical). */
#define RA3_EFFECT_GPU_SCALE 0.03f
#if RA3_TARGET_FPS == 120
#define RA3_EFFECT_CLIENT_SCALE 0.12f
#elif RA3_TARGET_FPS == 90
#define RA3_EFFECT_CLIENT_SCALE 0.09f
#elif RA3_TARGET_FPS == 60
#define RA3_EFFECT_CLIENT_SCALE 0.06f
#else
#define RA3_EFFECT_CLIENT_SCALE 0.03f
#endif

#endif /* RA3_RATE_CONFIG_H */
