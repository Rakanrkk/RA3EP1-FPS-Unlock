/* Authored 30 Hz suspension with render-only interpolation for one EP1 build.
 * Original game SHA-256:
 * 19b3542a7d2f7a050b16b2ea5e4ba0231b13c3fb7fcdb2c3e89767d4d354430b
 * Native sites were independently checked; no upstream body is copied.
 * The fresh caller buffer is never restored before native calculation. Cache
 * storage is owned by this DLL, including for native states created before
 * installation. Function A's actual output path determines field ownership. */
#ifndef SWAY_COMPAT_H
#define SWAY_COMPAT_H

#include <stdint.h>
#include <string.h>
#include "rate_config.h"

#define SWAY_GUARD_VA 0x005395a9u
#define SWAY_JE_VA 0x005395b7u
#define SWAY_FETCH_VA 0x005395a1u
#define SWAY_SKIP_VA 0x0053963du
#define SWAY_GETFRAME_GLOBAL 0x00ceeff8u
#define SWAY_STATE_OFF 0x208
#define SWAY_ALLOC_VA 0x00416930u
#define SWAY_STATE_OLD 0x70
#define SWAY_STATE_NEW 0x70
#define SWAY_FPS RA3_TARGET_FPS

static const uint8_t SWAY_GUARD_EXPECT[6] = {0x8b,0x50,0x74,0x55,0xff,0xd2};
static const uint8_t SWAY_JE_EXPECT[6] = {0x0f,0x84,0x80,0x00,0x00,0x00};
static const uint8_t SWAY_FETCH_EXPECT[8] = {0x8b,0x0d,0xf8,0xef,0xce,0x00,0x8b,0x01};
static const uint8_t SWAY_SKIP_EXPECT[9] = {0x5d,0x5e,0x5f,0x8a,0xc3,0x5b,0xc2,0x04,0x00};
typedef struct { uint32_t site; uint8_t expect[5]; uint32_t target; } sway_site_t;
static const sway_site_t SWAY_CALL_SITES[4] = {
    {0x5395eeu,{0xe8,0xdd,0x4e,0xff,0xff},0x52e4d0u},
    {0x539606u,{0xe8,0x25,0xd3,0xfe,0xff},0x526930u},
    {0x53961eu,{0xe8,0xad,0x4e,0xff,0xff},0x52e4d0u},
    {0x539636u,{0xe8,0x95,0x45,0xff,0xff},0x52dbd0u}
};
/* Identity inventory only: native allocations and save layout remain 0x70. */
static const sway_site_t SWAY_ALLOC_SITES[4] = {
    {0x52693eu,{0xe8,0xed,0xff,0xee,0xff},SWAY_ALLOC_VA},
    {0x52dbe4u,{0xe8,0x47,0x8d,0xee,0xff},SWAY_ALLOC_VA},
    {0x52e4e3u,{0xe8,0x48,0x84,0xee,0xff},SWAY_ALLOC_VA},
    {0x542558u,{0xe8,0xd3,0x43,0xed,0xff},SWAY_ALLOC_VA}
};

static inline uint32_t sway_frame30_fixed120(uint32_t frame) {
    return frame / 4u + (frame % 4u != 0);
}
static inline uint32_t sway_frame30_generic(uint32_t frame, uint32_t fps) {
    uint32_t period = fps / 30u;
    return frame / period + (frame % period != 0);
}
static inline uint32_t sway_frame30_fixed(uint32_t frame) {
    return sway_frame30_generic(frame, RA3_TARGET_FPS);
}
#if RA3_SWAY_STEPS == 4
static const float SWAY_BETA[4] = {0.25f,0.5f,0.75f,1.0f};
#elif RA3_SWAY_STEPS == 3
static const float SWAY_BETA[3] = {1.0f/3.0f,2.0f/3.0f,1.0f};
#elif RA3_SWAY_STEPS == 2
static const float SWAY_BETA[2] = {0.5f,1.0f};
#else
static const float SWAY_BETA[1] = {1.0f};
#endif
static inline float sway_beta_of(uint32_t frame) {
    unsigned phase = frame ? (frame - 1u) % RA3_SWAY_STEPS : RA3_SWAY_STEPS - 1u;
    return SWAY_BETA[phase];
}
static inline void sway_blend_mask(const uint32_t prev[4], const uint32_t cur[4],
                                   float beta, unsigned mask, uint32_t out[4]) {
    for (unsigned i = 0; i < 4; ++i) {
        if (!(mask & (1u << i))) continue;
        float p, c, value;
        memcpy(&p, prev + i, 4);
        memcpy(&c, cur + i, 4);
        value = p + (c - p) * beta;
        memcpy(out + i, &value, 4);
    }
}
static inline void sway_blend(const uint32_t prev[4], const uint32_t cur[4],
                              float beta, uint32_t out[4]) {
    sway_blend_mask(prev, cur, beta, 15, out);
}
static inline int32_t sway_rel32(uint32_t at, uint32_t to) {
    return (int32_t)(to - (at + 4u));
}
static inline uint32_t sway_rel32_target(uint32_t at, int32_t rel) {
    return at + 4u + (uint32_t)rel;
}

#if defined(_WIN32) && defined(__i386__)
/* Volatile targets let the owned i686 fixture substitute native callees. */
__attribute__((used)) static volatile uintptr_t sway_skip_continue = SWAY_SKIP_VA;
__attribute__((used)) static volatile uintptr_t sway_fn_a = 0x52e4d0u;
__attribute__((used)) static volatile uintptr_t sway_fn_b = 0x526930u;
__attribute__((used)) static volatile uintptr_t sway_fn_c = 0x52dbd0u;
__attribute__((used)) static volatile uintptr_t sway_a_short_continue = 0x52e6cdu;
__attribute__((used)) static volatile uintptr_t sway_a_full_continue = 0x52eb04u;
__attribute__((used)) static volatile uintptr_t sway_destroy_continue = 0x525f05u;
__attribute__((used)) static volatile uintptr_t sway_entry_continue = 0x53957au;
static const unsigned sway_authored_fps = 30;

typedef struct {
    uint32_t current[4], previous[4];
    unsigned filled, mask, type;
    void *locomotor, *object;
} SwayPoseCache;
typedef struct SwayCacheEntry {
    struct SwayCacheEntry *next;
    void *state, *drawable;
    SwayPoseCache pose;
} SwayCacheEntry;
static SRWLOCK sway_cache_lock = SRWLOCK_INIT;
static SwayCacheEntry *sway_cache_buckets[256];

typedef struct SwayOutputCall {
    struct SwayOutputCall *previous;
    void *drawable;
    unsigned mask;
} SwayOutputCall;
static DWORD sway_output_tls = TLS_OUT_OF_INDEXES;

static unsigned sway_raw_frame(void) {
    void *client = *(void **)SWAY_GETFRAME_GLOBAL;
    uintptr_t *vtable = *(uintptr_t **)client;
    typedef unsigned (__attribute__((thiscall)) *GetFrame)(void *);
    return ((GetFrame)vtable[0x74 / 4])(client);
}
static unsigned sway_bucket(void *state) {
    return ((uintptr_t)state >> 4u) & 255u;
}
/* Caller holds the exclusive lock. Never inspect or extend the native block. */
static SwayCacheEntry *sway_cache_entry(void *drawable, int create) {
    void *state = *(void **)((BYTE *)drawable + SWAY_STATE_OFF);
    if (!state) return NULL;
    SwayCacheEntry **head = &sway_cache_buckets[sway_bucket(state)];
    for (SwayCacheEntry *e = *head; e; e = e->next)
        if (e->state == state && e->drawable == drawable) return e;
    if (!create) return NULL;
    SwayCacheEntry *e = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*e));
    if (!e) return NULL;
    e->state = state;
    e->drawable = drawable;
    e->next = *head;
    *head = e;
    return e;
}
__attribute__((used,noinline)) static void sway_forget_state(void *state) {
    AcquireSRWLockExclusive(&sway_cache_lock);
    SwayCacheEntry **link = &sway_cache_buckets[sway_bucket(state)];
    while (*link) {
        SwayCacheEntry *e = *link;
        if (e->state == state) {
            *link = e->next;
            HeapFree(GetProcessHeap(), 0, e);
        } else link = &e->next;
    }
    ReleaseSRWLockExclusive(&sway_cache_lock);
}
static unsigned sway_native_type(void *loco) {
    BYTE *a = *(BYTE **)((BYTE *)loco + 4);
    BYTE *parameters = *(BYTE **)(a + 4);
    return *(unsigned *)(parameters + 0x64);
}
static int sway_active_type(unsigned type) {
    return type == 1 || type == 2 || type == 3 || type == 4 || type == 7 || type == 8;
}
static void *sway_object(void *drawable) {
    return *(void **)((BYTE *)drawable + 0x138);
}
/* These raw prerequisites are the native no-output guards, not a duplicate
 * call to the protected runtime predicate selecting A's short/full path. */
static int sway_output_eligible(void *object, unsigned type) {
    if (!object) return 0;
    if (type != 8 && !*(void **)((BYTE *)object + 0x384)) return 0;
    if ((type == 1 || type == 7) && !*(void **)((BYTE *)object + 0x388)) return 0;
    return 1;
}
static int sway_cache_matches(const SwayPoseCache *cache, void *loco,
                              unsigned type, void *object) {
    return cache->filled && cache->type == type && cache->locomotor == loco &&
           cache->object == object;
}
/* Run before the native dispatcher's early exits as well as its stamp guard. */
__attribute__((used,noinline)) static void sway_check_entry(void *drawable) {
    void *object = sway_object(drawable), *loco = NULL;
    unsigned type = 0;
    if (object) {
        BYTE *owner = *(BYTE **)((BYTE *)object + 0x384);
        if (owner) loco = *(void **)(owner + 0x228);
        if (loco) type = sway_native_type(loco);
    }
    AcquireSRWLockExclusive(&sway_cache_lock);
    SwayCacheEntry *entry = sway_cache_entry(drawable, 0);
    if (entry && (!loco || !sway_active_type(type) ||
                  !sway_output_eligible(object, type) ||
                  !sway_cache_matches(&entry->pose, loco, type, object)))
        entry->pose.filled = 0;
    ReleaseSRWLockExclusive(&sway_cache_lock);
}

static void sway_capture_pose(void *drawable, void *loco, float *pose, unsigned mask) {
    unsigned type = sway_native_type(loco);
    void *object = sway_object(drawable);
    unsigned frame = sway_raw_frame();
    if (!sway_active_type(type) || !sway_output_eligible(object, type)) mask = 0;
    AcquireSRWLockExclusive(&sway_cache_lock);
    SwayCacheEntry *entry = sway_cache_entry(drawable, 1);
    if (entry) {
        SwayPoseCache *cache = &entry->pose;
        int same = sway_cache_matches(cache, loco, type, object) && cache->mask == mask;
        for (unsigned i = 0; i < 4; ++i) {
            if (!(mask & (1u << i))) continue;
            uint32_t sample;
            memcpy(&sample, pose + i, 4);
            cache->previous[i] = same ? cache->current[i] : sample;
            cache->current[i] = sample;
        }
        cache->mask = mask;
        cache->type = type;
        cache->locomotor = loco;
        cache->object = object;
        cache->filled = mask != 0;
        if (mask) sway_blend_mask(cache->previous, cache->current, sway_beta_of(frame),
                                  mask, (uint32_t *)pose);
    }
    ReleaseSRWLockExclusive(&sway_cache_lock);
}

/* A missing/changed projection never authorizes another physics step. The
 * fresh pose survives until the next scheduled native call seeds that mode. */
__attribute__((used,noinline)) static unsigned sway_cached_pose(void *drawable, void *loco, float *pose) {
    unsigned type = sway_native_type(loco);
    void *object = sway_object(drawable);
    unsigned frame = sway_raw_frame(), result = 0;
    AcquireSRWLockExclusive(&sway_cache_lock);
    SwayCacheEntry *entry = sway_cache_entry(drawable, 0);
    if (entry) {
        SwayPoseCache *cache = &entry->pose;
        if (!sway_active_type(type) || !sway_output_eligible(object, type) ||
            !sway_cache_matches(cache, loco, type, object)) {
            cache->filled = 0;
        } else {
            sway_blend_mask(cache->previous, cache->current, sway_beta_of(frame),
                            cache->mask, (uint32_t *)pose);
            result = 1;
        }
    }
    ReleaseSRWLockExclusive(&sway_cache_lock);
    return result;
}
__attribute__((used,noinline)) static void sway_record_a_path(void *drawable, unsigned mask) {
    SwayOutputCall *call = TlsGetValue(sway_output_tls);
    if (call && call->drawable == drawable) call->mask = mask;
}

/* Internal native blocks can carry live x87 and SSE values. Preserve both,
 * in addition to flags/GPRs, around helpers. EBP addresses the saved GPRs. */
#define SWAY_SAVE_CONTEXT \
    "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\t" \
    "fxsave (%esp)\n\tsubl $16,%esp\n\t"
#define SWAY_RESTORE_CONTEXT \
    "addl $16,%esp\n\tfxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\t"

__attribute__((naked,used)) static void sway_entry_bridge(void) {
    __asm__ __volatile__(SWAY_SAVE_CONTEXT
        "movl %edi,(%esp)\n\tcall _sway_check_entry\n\t"
        SWAY_RESTORE_CONTEXT
        "movl 0x138(%edi),%eax\n\tjmp *_sway_entry_continue\n\t");
}
__attribute__((naked,used)) static void sway_a_short_bridge(void) {
    __asm__ __volatile__(SWAY_SAVE_CONTEXT
        "movl %esi,(%esp)\n\tmovl $11,4(%esp)\n\tcall _sway_record_a_path\n\t"
        SWAY_RESTORE_CONTEXT
        "movl 0x208(%esi),%edx\n\tjmp *_sway_a_short_continue\n\t");
}
__attribute__((naked,used)) static void sway_a_full_bridge(void) {
    __asm__ __volatile__(SWAY_SAVE_CONTEXT
        "movl %esi,(%esp)\n\tmovl $15,4(%esp)\n\tcall _sway_record_a_path\n\t"
        SWAY_RESTORE_CONTEXT
        "movss 0x1c(%eax),%xmm0\n\tjmp *_sway_a_full_continue\n\t");
}
__attribute__((naked,used,thiscall)) static void *sway_destroy_bridge(void *state __attribute__((unused)), unsigned flags __attribute__((unused))) {
    __asm__ __volatile__(SWAY_SAVE_CONTEXT
        "movl %ecx,(%esp)\n\tcall _sway_forget_state\n\t"
        SWAY_RESTORE_CONTEXT
        "testb $1,4(%esp)\n\tjmp *_sway_destroy_continue\n\t");
}

/* Preserves the dispatcher's saved EBP push and returns frame30 in EAX. */
__attribute__((used,noinline)) static unsigned sway_get_authored_frame(void) {
    return sway_frame30_fixed(sway_raw_frame());
}
__attribute__((naked,used)) static void sway_frame30_bridge(void) {
    __asm__ __volatile__("jmp _sway_get_authored_frame\n\t");
}
__attribute__((naked,used)) static void sway_cache_bridge(void) {
    __asm__ __volatile__(SWAY_SAVE_CONTEXT
        "movl 0x38(%ebp),%eax\n\tmovl %eax,8(%esp)\n\tmovl %esi,4(%esp)\n\t"
        "movl %edi,(%esp)\n\tcall _sway_cached_pose\n\tmovl %eax,0x1c(%ebp)\n\t"
        SWAY_RESTORE_CONTEXT
        "testl %eax,%eax\n\tje 1f\n\tmovb $1,%bl\n\t1: jmp *_sway_skip_continue\n\t");
}

typedef void (__attribute__((thiscall)) *SwayNativeStep)(void *, void *, float *);
static void __attribute__((used,thiscall)) sway_thunk_a(void *drawable, void *loco, float *pose) {
    SwayOutputCall call = {TlsGetValue(sway_output_tls), drawable, 0};
    int tracking = TlsSetValue(sway_output_tls, &call);
    ((SwayNativeStep)sway_fn_a)(drawable, loco, pose);
    if (tracking) TlsSetValue(sway_output_tls, call.previous);
    sway_capture_pose(drawable, loco, pose, tracking ? call.mask : 0);
}
static void __attribute__((used,thiscall)) sway_thunk_b(void *drawable, void *loco, float *pose) {
    ((SwayNativeStep)sway_fn_b)(drawable, loco, pose);
    sway_capture_pose(drawable, loco, pose, 10);
}
static void __attribute__((used,thiscall)) sway_thunk_c(void *drawable, void *loco, float *pose) {
    ((SwayNativeStep)sway_fn_c)(drawable, loco, pose);
    sway_capture_pose(drawable, loco, pose, 15);
}

static BOOL sway_add_jump(uintptr_t site, const BYTE *expected, unsigned length, void *bridge) {
    BYTE bytes[16];
    if (length < 5 || length > sizeof(bytes)) return FALSE;
    memset(bytes, 0x90, length);
    bytes[0] = 0xe9;
    int32_t relative = sway_rel32((uint32_t)site + 1u, (uint32_t)(uintptr_t)bridge);
    memcpy(bytes + 1, &relative, 4);
    return add_edit(site, expected, bytes, length);
}
static BOOL sway_prepare_edits(void) {
    if (sway_output_tls == TLS_OUT_OF_INDEXES) sway_output_tls = TlsAlloc();
    if (sway_output_tls == TLS_OUT_OF_INDEXES) return FALSE;
    if (!add_edit(SWAY_FETCH_VA, SWAY_FETCH_EXPECT, SWAY_FETCH_EXPECT, sizeof(SWAY_FETCH_EXPECT)) ||
        !add_edit(SWAY_SKIP_VA, SWAY_SKIP_EXPECT, SWAY_SKIP_EXPECT, sizeof(SWAY_SKIP_EXPECT))) return FALSE;
    if (!add_call(SWAY_CALL_SITES[0].site, SWAY_CALL_SITES[0].expect, sway_thunk_a) ||
        !add_call(SWAY_CALL_SITES[1].site, SWAY_CALL_SITES[1].expect, sway_thunk_b) ||
        !add_call(SWAY_CALL_SITES[2].site, SWAY_CALL_SITES[2].expect, sway_thunk_a) ||
        !add_call(SWAY_CALL_SITES[3].site, SWAY_CALL_SITES[3].expect, sway_thunk_c)) return FALSE;
    const BYTE short_output[] = {0x8b,0x96,0x08,0x02,0x00,0x00};
    const BYTE full_output[] = {0xf3,0x0f,0x10,0x40,0x1c};
    const BYTE destroy[] = {0xf6,0x44,0x24,0x04,0x01};
    const BYTE entry[] = {0x8b,0x87,0x38,0x01,0x00,0x00};
    if (!sway_add_jump(0x52e6c7, short_output, sizeof(short_output), sway_a_short_bridge) ||
        !sway_add_jump(0x52eaff, full_output, sizeof(full_output), sway_a_full_bridge) ||
        !sway_add_jump(0x525f00, destroy, sizeof(destroy), sway_destroy_bridge) ||
        !sway_add_jump(0x539574, entry, sizeof(entry), sway_entry_bridge)) return FALSE;
    const BYTE rate_reads[2][6] = {{0x8b,0x0d,0x9c,0x3a,0xcc,0x00},
                                  {0xdb,0x05,0x9c,0x3a,0xcc,0x00}};
    const uintptr_t rate_sites[] = {0x52debd, 0x52dec3};
    for (unsigned i = 0; i < 2; ++i) {
        BYTE bytes[6];
        uint32_t address = (uint32_t)(uintptr_t)&sway_authored_fps;
        memcpy(bytes, rate_reads[i], sizeof(bytes));
        memcpy(bytes + 2, &address, 4);
        if (!add_edit(rate_sites[i], rate_reads[i], bytes, sizeof(bytes))) return FALSE;
    }
    BYTE bytes[6] = {0x0f,0x84,0,0,0,0};
    int32_t relative = sway_rel32(SWAY_JE_VA + 2, (uint32_t)(uintptr_t)sway_cache_bridge);
    memcpy(bytes + 2, &relative, 4);
    if (!add_edit(SWAY_JE_VA, SWAY_JE_EXPECT, bytes, sizeof(bytes))) return FALSE;
    bytes[0] = 0x55; bytes[1] = 0xe8;
    relative = sway_rel32(SWAY_GUARD_VA + 2, (uint32_t)(uintptr_t)sway_frame30_bridge);
    memcpy(bytes + 2, &relative, 4);
    return add_edit(SWAY_GUARD_VA, SWAY_GUARD_EXPECT, bytes, sizeof(bytes));
}
#undef SWAY_SAVE_CONTEXT
#undef SWAY_RESTORE_CONTEXT
#endif
#endif
