/* Exercise actual i686 bridges with independent native output postconditions. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct { uintptr_t site; BYTE bytes[16]; SIZE_T size; } RecordedEdit;
static RecordedEdit recorded[40];
static unsigned recorded_count;
static BOOL add_edit(uintptr_t site, const void *before, const void *after, SIZE_T size) {
    (void)before;
    if (recorded_count == 40 || size > 16) return FALSE;
    RecordedEdit *edit = &recorded[recorded_count++];
    edit->site = site;
    edit->size = size;
    memcpy(edit->bytes, after, size);
    return TRUE;
}
static BOOL add_call(uintptr_t site, const BYTE *before, void *bridge) {
    BYTE bytes[5] = {0xe8, 0, 0, 0, 0};
    uint32_t relative = (uint32_t)((uintptr_t)bridge - site - 5);
    memcpy(bytes + 1, &relative, 4);
    return add_edit(site, before, bytes, sizeof(bytes));
}
#include "sway_compat.h"

static unsigned frame_number, native_steps, failures, a_path = 2;
static BYTE drawable[0x210], locomotor[0x100], loco_link[8], parameters[0x180];
static BYTE object[0x390], other_object[0x390], owner[0x230];
static void *native_state;
static float output[4];
__attribute__((used)) static unsigned cache_return;
__attribute__((used)) static volatile uintptr_t cache_target;
__attribute__((used)) static BYTE deletion_requested;
__attribute__((used,aligned(16))) static const float vector_seed[4] = {1,2,3,4};
__attribute__((used)) static float vector_after[4], x87_after;

static void check(int good, const char *what) {
    if (!good) { printf("FAIL: %s\n", what); failures++; }
}
static unsigned __attribute__((thiscall)) fake_frame(void *self) {
    (void)self;
    return frame_number;
}
__attribute__((naked)) static void fake_rejoin(void) {
    __asm__ __volatile__("ret\n\t");
}
/* Native output blocks have ESI=drawable; the full path has EAX=state.
 * Keep x87 and another SSE register live across the real recording bridge. */
__attribute__((naked)) static void call_output_bridge(void *self __attribute__((unused)), void *state __attribute__((unused)), uintptr_t bridge __attribute__((unused))) {
    __asm__ __volatile__(
        "pushl %esi\n\tmovl 8(%esp),%esi\n\tmovl 12(%esp),%eax\n\t"
        "fldpi\n\tmovups _vector_seed,%xmm1\n\tcall *16(%esp)\n\t"
        "fstps _x87_after\n\tmovups %xmm1,_vector_after\n\tpopl %esi\n\tret\n\t");
}
static void fake_step(void *self, void *loco, float *pose, unsigned mask) {
    check(self == drawable, "original ECX=this forwarded");
    check(loco == locomotor, "first stack argument forwarded");
    check(pose == output, "second stack argument forwarded");
    native_steps++;
    if (mask & 1) pose[0] += 4.0f;
    if (mask & 2) pose[1] -= 8.0f;
    if (mask & 4) pose[2] += 12.0f;
    if (mask & 8) pose[3] += 16.0f;
}
static void __attribute__((thiscall)) fake_step_a(void *self, void *loco, float *pose) {
    if (!a_path) {
        fake_step(self, loco, pose, 0);
        return;
    }
    unsigned mask = a_path == 1 ? 11u : 15u;
    call_output_bridge(self, native_state,
                       a_path == 1 ? (uintptr_t)sway_a_short_bridge : (uintptr_t)sway_a_full_bridge);
    check(fabsf(x87_after - 3.14159274f) < 0.00001f, "native x87 value survives output bridge");
    check(!memcmp(vector_after, vector_seed, sizeof(vector_after)), "native SSE value survives output bridge");
    fake_step(self, loco, pose, mask);
}
static void __attribute__((thiscall)) fake_step_b(void *self, void *loco, float *pose) {
    fake_step(self, loco, pose, 10u);
}
static void __attribute__((thiscall)) fake_step_c(void *self, void *loco, float *pose) {
    fake_step(self, loco, pose, 15u);
}
__attribute__((naked)) static void fake_epilogue(void) {
    __asm__ __volatile__("popl %ebp\n\tpopl %esi\n\tpopl %edi\n\tmovb %bl,%al\n\tpopl %ebx\n\tret $4\n\t");
}
__attribute__((naked)) static void fake_destroy_tail(void) {
    __asm__ __volatile__("setnz _deletion_requested\n\tmovl %ecx,%eax\n\tret $4\n\t");
}
__attribute__((naked)) static void *call_entry_bridge(void *self __attribute__((unused))) {
    __asm__ __volatile__(
        "pushl %edi\n\tmovl 8(%esp),%edi\n\tcall _sway_entry_bridge\n\tpopl %edi\n\tret\n\t");
}
__attribute__((naked)) static void call_cache(void *self __attribute__((unused)), float *pose __attribute__((unused))) {
    __asm__ __volatile__(
        "pushl %ebx\n\tpushl %esi\n\tpushl %edi\n\tpushl %ebp\n\t"
        "movl 0x14(%esp),%edi\n\tmovl 0x18(%esp),%edx\n\t"
        "pushl %edx\n\tcall 1f\n\tjmp 2f\n\t"
        "1: pushl %ebx\n\tpushl %edi\n\tpushl %esi\n\tpushl %ebp\n\t"
        "movl $_locomotor,%esi\n\txorl %ebx,%ebx\n\tjmp *_cache_target\n\t"
        "2: movzbl %al,%eax\n\tmovl %eax,_cache_return\n\t"
        "popl %ebp\n\tpopl %edi\n\tpopl %esi\n\tpopl %ebx\n\tret\n\t");
}
static void call_step(uintptr_t address) {
    typedef void (__attribute__((thiscall)) *Step)(void *, void *, float *);
    Step step;
    memcpy(&step, &address, sizeof(step));
    step(drawable, locomotor, output);
}
static void check_pose(const float *want, const char *what) {
    for (unsigned i = 0; i < 4; ++i) {
        if (!isfinite(output[i]) || fabsf(output[i] - want[i]) > 0.0001f) {
            printf("FAIL: %s field %u actual=%f expected=%f\n", what, i, output[i], want[i]);
            failures++;
        }
    }
}
static void set_object(BYTE *value) {
    *(void **)(drawable + 0x138) = value;
    *(void **)(value + 0x384) = owner;
    *(void **)(owner + 0x228) = locomotor;
    *(void **)(value + 0x388) = parameters;
}
static void clear_cache(void) {
    sway_forget_state(native_state);
}

int main(void) {
    void *mapped = VirtualAlloc((void *)0x00ce0000, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    check(mapped == (void *)0x00ce0000, "owned fake GameClient address mapped");
    if (!mapped) return 1;
    static uintptr_t vtable[32], client;
    vtable[0x74 / 4] = (uintptr_t)fake_frame;
    client = (uintptr_t)vtable;
    *(uintptr_t *)0x00ceeff8 = (uintptr_t)&client;
    sway_fn_a = (uintptr_t)fake_step_a;
    sway_fn_b = (uintptr_t)fake_step_b;
    sway_fn_c = (uintptr_t)fake_step_c;
    sway_skip_continue = (uintptr_t)fake_epilogue;
    sway_a_short_continue = sway_a_full_continue = (uintptr_t)fake_rejoin;
    sway_entry_continue = (uintptr_t)fake_rejoin;
    sway_destroy_continue = (uintptr_t)fake_destroy_tail;
    *(void **)(locomotor + 4) = loco_link;
    *(void **)(loco_link + 4) = parameters;
    *(unsigned *)(parameters + 0x64) = 1;
    parameters[0x17b] = 0; /* Full native output despite this flag being clear. */
    set_object(object);
    cache_target = (uintptr_t)sway_cache_bridge;
    check(sway_prepare_edits(), "all guarded edits prepare");
    check(recorded_count == 14, "exact suspension edit count");
    for (unsigned i = 0; i < recorded_count; ++i) {
        RecordedEdit *edit = &recorded[i];
        if (edit->site == SWAY_GUARD_VA) {
            check(edit->size == 6 && edit->bytes[0] == 0x55 && edit->bytes[1] == 0xe8,
                  "guard encodes push ebp then CALL rel32");
            int32_t relative;
            memcpy(&relative, edit->bytes + 2, 4);
            check((uint32_t)(SWAY_GUARD_VA + 6 + relative) == (uint32_t)(uintptr_t)sway_frame30_bridge,
                  "guard CALL resolves to actual bridge");
        }
        if (edit->site == 0x52debd || edit->site == 0x52dec3) {
            uint32_t address;
            memcpy(&address, edit->bytes + 2, 4);
            check(address == (uintptr_t)&sway_authored_fps && *(unsigned *)address == 30,
                  "C reads private authored denominator rather than shared client FPS");
        }
    }

    /* A native-sized state ends immediately before an inaccessible page.
     * The previous tail cache faults here; the external cache never reads it. */
    BYTE *pages = VirtualAlloc(NULL, 0x2000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DWORD old_protection;
    check(pages && VirtualProtect(pages + 0x1000, 0x1000, PAGE_NOACCESS, &old_protection),
          "legacy native-sized state has a protected end boundary");
    if (!pages) return 1;
    native_state = pages + 0x1000 - SWAY_STATE_OLD;
    memset(native_state, 0xcd, SWAY_STATE_OLD);
    *(void **)(drawable + SWAY_STATE_OFF) = native_state;
    BYTE original_state[SWAY_STATE_OLD];
    memcpy(original_state, native_state, sizeof(original_state));

    float initial[4] = {1,2,3,4};
    memcpy(output, initial, sizeof(output));
    frame_number = 1;
    call_step((uintptr_t)sway_thunk_a);
    float first[4] = {5,-6,15,20};
    check_pose(first, "first native sample does not blend from zero");
    const unsigned period = SWAY_FPS / 30;
    for (unsigned f = 2; f <= period; ++f) {
        frame_number = f;
        memset(output, 0, sizeof(output));
        call_cache(drawable, output);
        check(cache_return == 1, "actual A full output has a cache");
        check_pose(first, "full native heave survives fresh cached displays");
    }
    frame_number = period + 1;
    memcpy(output, first, sizeof(output));
    call_step((uintptr_t)sway_thunk_a);
    const float delta[4] = {4,-8,12,16};
    for (unsigned f = period + 1; f <= 2 * period; ++f) {
        frame_number = f;
        if (f != period + 1) {
            memset(output, 0, sizeof(output));
            call_cache(drawable, output);
        }
        float want[4];
        for (unsigned k = 0; k < 4; ++k) want[k] = first[k] + delta[k] * (float)(f - period) / period;
        check_pose(want, "native sample interpolation follows each display phase");
    }

    /* The native short/full outcome is independent of parameter+17b. */
    a_path = 1;
    parameters[0x17b] = 1;
    frame_number = 2 * period + 1;
    float short_pose[4] = {100,200,777,400};
    memcpy(output, short_pose, sizeof(output));
    call_step((uintptr_t)sway_thunk_a);
    short_pose[0] += 4; short_pose[1] -= 8; short_pose[3] += 16;
    check_pose(short_pose, "full-to-short transition reseeds and preserves fresh height");
    output[2] = 888;
    frame_number++;
    call_cache(drawable, output);
    short_pose[2] = 888;
    check_pose(short_pose, "short branch never caches caller-owned height");

    a_path = 0;
    frame_number = 3 * period + 1;
    memset(output, 0, sizeof(output));
    call_step((uintptr_t)sway_thunk_a);
    float zero[4] = {0,0,0,0};
    check_pose(zero, "no-output native return does not blend stale tilt");
    frame_number++;
    call_cache(drawable, output);
    check(cache_return == 0, "no-output sample invalidates history");
    check_pose(zero, "no-output cache leaves fresh pose unchanged");

    a_path = 2;
    frame_number = 4 * period + 1;
    call_step((uintptr_t)sway_thunk_a);
    unsigned before = native_steps;
    *(void **)(object + 0x388) = NULL;
    memset(output, 0, sizeof(output));
    frame_number++;
    call_cache(drawable, output);
    check(cache_return == 0, "A missing native prerequisite invalidates cache");
    check_pose(zero, "prerequisite loss leaves the caller pose unchanged");
    *(void **)(object + 0x388) = parameters;
    *(unsigned *)(parameters + 0x64) = 2;
    call_cache(drawable, output);
    check(cache_return == 0 && native_steps == before, "mode change never integrates twice inside a tick");
    set_object(other_object);
    call_cache(drawable, output);
    check(cache_return == 0, "object replacement cannot reuse earlier history");
    set_object(object);

    /* The real dispatcher can exit before its frame/stamp/cache guard. */
    *(unsigned *)(parameters + 0x64) = 1;
    frame_number = 8 * period + 1;
    call_step((uintptr_t)sway_thunk_a);
    *(void **)(object + 0x384) = NULL;
    check(call_entry_bridge(drawable) == object, "entry bridge replays the native object load");
    set_object(object);
    memset(output, 0, sizeof(output));
    call_cache(drawable, output);
    check(cache_return == 0, "native early exit invalidates before same-identity re-entry");
    check_pose(zero, "re-entry never resurrects a pose invalidated at native entry");

    /* Literal targets from the pinned native type-minus-one jump table. */
    static const uint32_t dispatch[8] = {
        0x5395e6,0x5395fe,0x5395fe,0x5395fe,0x53963d,0x53963d,0x539616,0x53962e
    };
    for (unsigned type = 1; type <= 8; ++type) {
        for (unsigned path = 1; path <= 2; ++path) {
            for (unsigned flag = 0; flag <= 1; ++flag) {
                clear_cache();
                *(unsigned *)(parameters + 0x64) = type;
                parameters[0x17b] = (BYTE)flag;
                a_path = path;
                frame_number = 12 * period + 1;
                float fresh[4] = {101,202,303,404}, want[4];
                memcpy(want, fresh, sizeof(want));
                memcpy(output, fresh, sizeof(output));
                uintptr_t thunk = 0;
                unsigned mask = 0;
                switch (dispatch[type - 1]) {
                case 0x5395e6: case 0x539616:
                    thunk = (uintptr_t)sway_thunk_a; mask = path == 1 ? 11u : 15u; break;
                case 0x5395fe:
                    thunk = (uintptr_t)sway_thunk_b; mask = 10u; break;
                case 0x53962e:
                    thunk = (uintptr_t)sway_thunk_c; mask = 15u; break;
                }
                if (!thunk) {
                    call_cache(drawable, output);
                    check(cache_return == 0, "types 5/6 skip suspension");
                    check_pose(fresh, "skip branch preserves every fresh field");
                    continue;
                }
                call_step(thunk);
                for (unsigned k = 0; k < 4; ++k) if (mask & (1u << k)) want[k] += delta[k];
                check_pose(want, "each native type captures its actual output fields");
                before = native_steps;
                frame_number++;
                for (unsigned k = 0; k < 4; ++k) {
                    output[k] = fresh[k] + 1000;
                    if (!(mask & (1u << k))) want[k] = output[k];
                }
                call_cache(drawable, output);
                check(cache_return == 1, "each active native type has a valid cache");
                check_pose(want, "cached native types preserve fresh caller fields");
                check(native_steps == before, "cached output adds no native steps");
            }
        }
    }
    check(!memcmp(native_state, original_state, sizeof(original_state)), "cache code leaves the original native bytes unchanged");

    /* Destructor bridge forgets all cache ownership before native deletion,
     * and replays the flags test that decides whether native memory is freed. */
    *(unsigned *)(parameters + 0x64) = 1;
    a_path = 2;
    frame_number = 20 * period + 1;
    call_step((uintptr_t)sway_thunk_a);
    typedef void *(__attribute__((thiscall)) *Destroy)(void *, unsigned);
    Destroy destroy = (Destroy)sway_destroy_bridge;
    destroy(native_state, 1);
    check(deletion_requested == 1, "destructor deletion-bit flags replayed");
    memset(output, 0, sizeof(output));
    call_cache(drawable, output);
    check(cache_return == 0, "destroyed native address has no cached history");
    destroy(native_state, 0);
    check(deletion_requested == 0, "nondeleting destructor flags replayed");
    clear_cache();
    TlsFree(sway_output_tls);
    VirtualFree(pages, 0, MEM_RELEASE);
    VirtualFree(mapped, 0, MEM_RELEASE);
    if (!failures) printf("PASS: actual i686 suspension at %u FPS; native-size guard page, actual A outcomes, eight native types, x87/SSE, cache and destructor bridges.\n", SWAY_FPS);
    return failures ? 1 : 42;
}
