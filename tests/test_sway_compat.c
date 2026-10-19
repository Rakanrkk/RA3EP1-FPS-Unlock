/* Host test for the pure parts of sway_compat.h (quantization, blend schedule,
 * rel32 math, site tables, state layout). Run on macOS: cc test_sway_compat.c */
#include <stdio.h>
#include <string.h>
#include "sway_compat.h"

static int failures;

static void check(int ok, const char *what) {
    if (!ok) { printf("FAIL %s\n", what); ++failures; }
}

int main(void) {
    /* Compile-time authored clock agrees with an independent integer ratio. */
    for (uint32_t f = 0; f < 100000u; ++f)
        if (sway_frame30_fixed(f) != sway_frame30_generic(f, SWAY_FPS)) {
            printf("FAIL quantize at %u\n", f); ++failures; break;
        }
#if RA3_TARGET_FPS == 120
    /* The fixed120 helper pins the accepted 120 Hz quantizer; other rates
     * exercise the generic clock through the identity and phase loops. */
    check(sway_frame30_fixed120(0) == 0, "frame0");
    check(sway_frame30_fixed120(1) == 1 && sway_frame30_fixed120(4) == 1 &&
          sway_frame30_fixed120(5) == 2 && sway_frame30_fixed120(119) == 30 &&
          sway_frame30_fixed120(120) == 30, "quantize edges");
#endif

    /* One native step per authored period at each supported display rate. */
    for (uint32_t f = 1u; f < 64u; ++f)
        if ((((f - 1u) % RA3_SWAY_STEPS) == 0) !=
            (sway_frame30_fixed(f) != sway_frame30_fixed(f - 1u))) {
            printf("FAIL step phase at %u\n", f); ++failures; break;
        }

    /* blend schedule: one step of latency, ends on cur, never extrapolates */
    for (uint32_t f = 0; f < 32u; ++f) {
        float b = sway_beta_of(f);
        float want = (float)((f + RA3_SWAY_STEPS - 1u) % RA3_SWAY_STEPS + 1u) / RA3_SWAY_STEPS;
        if (!(b > 0.0f && b <= 1.0f)) want = -1.0f; /* keep bound check */
        if (b != want || b <= 0.0f || b > 1.0f) {
            printf("FAIL beta at %u\n", f); ++failures; break;
        }
    }

    /* component blend of four float scalars incl. negatives */
    { uint32_t p[4] = {0x3f800000u, 0xbf800000u, 0, 0x40490fdbu};
      uint32_t c[4] = {0x40000000u, 0x40000000u, 0xbf800000u, 0x40490fdbu};
      uint32_t o[4];
      sway_blend(p, c, 0.5f, o);
      float a, b, d, e;
      memcpy(&a, &o[0], 4); memcpy(&b, &o[1], 4); memcpy(&d, &o[2], 4); memcpy(&e, &o[3], 4);
      check(a == 1.5f && b == 0.5f && d == -0.5f && e == 3.14159274f, "blend values"); }

    /* rel32 round trip across negative and positive displacements */
    { uint32_t sites[] = {0x005395b9u, 0x005395a9u, 0x005395eeu};
      for (unsigned i = 0; i < 3; ++i)
        for (uint32_t to = 0x1000000u; to < 0x2000000u; to += 0x1234567u) {
            int32_t r = sway_rel32(sites[i] + 2u, to);
            if (sway_rel32_target(sites[i] + 2u, r) != to ||
                sites[i] + 6u + (uint32_t)r != to) {
                printf("FAIL rel32 %u\n", i); ++failures; goto done_rel;
            }
        }
    }
done_rel:
    /* guard replacement shape: 55 E8 rel32 where next instr = GUARD+6 */
    { int32_t r = sway_rel32(SWAY_GUARD_VA + 2, 0x30000000u);
      check(SWAY_GUARD_VA + 6 + (uint32_t)r == 0x30000000u, "guard rel32 base"); }

    /* site tables: expected first byte E8 and targets sane, four alloc sites
     * all target the one cdecl allocator, call sites cover 3 unique fns */
    check(sizeof(SWAY_CALL_SITES) / sizeof(*SWAY_CALL_SITES) == 4, "4 call sites");
    check(sizeof(SWAY_ALLOC_SITES) / sizeof(*SWAY_ALLOC_SITES) == 4, "4 alloc sites");
    for (unsigned i = 0; i < 4; ++i)
        check(SWAY_CALL_SITES[i].expect[0] == 0xE8 && SWAY_ALLOC_SITES[i].expect[0] == 0xE8,
              "E8 first byte");
    check(SWAY_CALL_SITES[0].target == SWAY_CALL_SITES[2].target, "fn A shared");
    check(SWAY_CALL_SITES[0].target != SWAY_CALL_SITES[1].target &&
          SWAY_CALL_SITES[1].target != SWAY_CALL_SITES[3].target, "distinct fns");
    for (unsigned i = 0; i < 4; ++i)
        check(SWAY_ALLOC_SITES[i].target == SWAY_ALLOC_VA, "alloc target");

    /* Native state and serialized fields retain their original allocation. */
    check(SWAY_STATE_OLD == 0x70 && SWAY_STATE_NEW == 0x70,
          "external cache never expands a native state");
    check(SWAY_GUARD_EXPECT[0] == 0x8B && SWAY_GUARD_EXPECT[1] == 0x50 &&
          SWAY_JE_EXPECT[0] == 0x0F && SWAY_JE_EXPECT[1] == 0x84, "opcode pins");

    printf(failures ? "sway_compat host tests: %d FAILURE(S)\n" : "sway_compat host tests: all pass\n",
           failures);
    return failures ? 1 : 0;
}
