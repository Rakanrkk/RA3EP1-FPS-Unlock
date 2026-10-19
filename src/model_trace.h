/* Bounded, diagnostic-only native model event journal. No gameplay writes. */
#ifndef RA3_MODEL_TRACE_H
#define RA3_MODEL_TRACE_H

#include <stdint.h>
#include <string.h>

#ifndef RA3_MODEL_TRACE
#define RA3_MODEL_TRACE 0
#endif

#define MODEL_TRACE_WORDS 20u
#define MODEL_TRACE_CAPACITY 4096u
#define MODEL_TRACE_LIMIT 524288u

typedef struct {
    uint32_t sequence, kind, wall_ms, display_frame;
    uint32_t client_frame, logic_frame, stage, fraction;
    uint32_t subject, timing_enabled, count, reserved;
    uint32_t words[MODEL_TRACE_WORDS];
} ModelTraceRecord;
_Static_assert(sizeof(ModelTraceRecord) == 128, "fixed native trace record");

typedef struct {
    char magic[8];
    uint32_t version, record_bytes, target_fps, capacity;
    uint32_t record_limit, started_ms, reserved[2];
} ModelTraceHeader;
_Static_assert(sizeof(ModelTraceHeader) == 40, "fixed native trace header");

enum {
    MODEL_TRACE_DRAWABLE = 1,
    MODEL_TRACE_CLIP = 2,
    MODEL_TRACE_TRANSITION = 3,
    MODEL_TRACE_CHANNEL = 4,
    MODEL_TRACE_COMMIT = 5,
    MODEL_TRACE_CLIP_AFTER = 6,
    MODEL_TRACE_COUNTERS = 0xffff
};

#if RA3_MODEL_TRACE && defined(_WIN32) && defined(__i386__)
static SRWLOCK model_trace_lock = SRWLOCK_INIT;
static ModelTraceRecord model_trace_ring[MODEL_TRACE_CAPACITY];
static unsigned model_trace_read_at, model_trace_queued, model_trace_total;
static volatile LONG model_trace_sequence, model_trace_dropped, model_trace_capped;
static FILE *model_trace_file;
static DWORD model_trace_started_ms;

static int model_trace_open(const wchar_t *path) {
    model_trace_file = _wfopen(path, L"wb");
    if (!model_trace_file) return 0;
    model_trace_started_ms = GetTickCount();
    ModelTraceHeader header = {{'R','A','3','T','R','C','1',0}, 1,
        sizeof(ModelTraceRecord), RA3_TARGET_FPS, MODEL_TRACE_CAPACITY,
        MODEL_TRACE_LIMIT, model_trace_started_ms, {0,0}};
    if (fwrite(&header, sizeof(header), 1, model_trace_file) != 1) {
        fclose(model_trace_file);
        model_trace_file = NULL;
        return 0;
    }
    fflush(model_trace_file);
    return 1;
}

/* Native hooks supply only verified fields. The journal owns its storage;
 * contended/full captures are dropped rather than waiting on disk or locks. */
static void model_trace_enqueue(ModelTraceRecord *record) {
    record->sequence = (uint32_t)InterlockedIncrement(&model_trace_sequence);
    if (!TryAcquireSRWLockExclusive(&model_trace_lock)) {
        InterlockedIncrement(&model_trace_dropped);
        return;
    }
    if (model_trace_total == MODEL_TRACE_LIMIT) {
        InterlockedIncrement(&model_trace_capped);
    } else if (model_trace_queued == MODEL_TRACE_CAPACITY) {
        InterlockedIncrement(&model_trace_dropped);
    } else {
        unsigned at = (model_trace_read_at + model_trace_queued) % MODEL_TRACE_CAPACITY;
        model_trace_ring[at] = *record;
        ++model_trace_queued;
        ++model_trace_total;
    }
    ReleaseSRWLockExclusive(&model_trace_lock);
}

static void model_trace_emit(unsigned kind, const void *subject,
                              const uint32_t *words, unsigned count) {
    if (!model_trace_file || model_trace_capped) return;
    ModelTraceRecord record = {0};
    record.kind = kind;
    record.wall_ms = GetTickCount() - model_trace_started_ms;
    record.display_frame = (uint32_t)display_frames;
    BYTE *client = *(BYTE **)0x00ceeff8u;
    BYTE *logic = *(BYTE **)0x00cec568u;
    if (client) memcpy(&record.client_frame, client + 0x48, 4);
    if (logic) memcpy(&record.logic_frame, logic + 0x50, 4);
    if (frame_context) {
        memcpy(&record.stage, frame_context + 0x58, 4);
        memcpy(&record.fraction, frame_context + 0x60, 4);
    }
    record.subject = (uint32_t)(uintptr_t)subject;
    record.timing_enabled = enabled != 0;
    record.count = count > MODEL_TRACE_WORDS ? MODEL_TRACE_WORDS : count;
    if (record.count) memcpy(record.words, words, record.count * 4);
    model_trace_enqueue(&record);
}

static void model_trace_flush(void) {
    if (!model_trace_file) return;
    ModelTraceRecord batch[64];
    for (;;) {
        AcquireSRWLockExclusive(&model_trace_lock);
        unsigned count = model_trace_queued > 64 ? 64 : model_trace_queued;
        for (unsigned i = 0; i < count; ++i)
            batch[i] = model_trace_ring[(model_trace_read_at + i) % MODEL_TRACE_CAPACITY];
        model_trace_read_at = (model_trace_read_at + count) % MODEL_TRACE_CAPACITY;
        model_trace_queued -= count;
        ReleaseSRWLockExclusive(&model_trace_lock);
        if (!count) break;
        if (fwrite(batch, sizeof(*batch), count, model_trace_file) != count) {
            /* Disable capture on an I/O failure; never stall the game. */
            InterlockedExchange(&model_trace_capped, 1);
            break;
        }
    }
    fflush(model_trace_file);
}

static void model_trace_counters(void) {
    uint32_t values[] = {(uint32_t)model_trace_dropped, (uint32_t)model_trace_capped,
                         model_trace_total, (uint32_t)model_trace_sequence};
    /* Counters remain observable after the capture budget is exhausted. */
    ModelTraceRecord record = {0};
    record.kind = MODEL_TRACE_COUNTERS;
    record.wall_ms = GetTickCount() - model_trace_started_ms;
    record.count = sizeof(values) / sizeof(*values);
    memcpy(record.words, values, sizeof(values));
    model_trace_flush();
    if (model_trace_file) {
        fwrite(&record, sizeof(record), 1, model_trace_file);
        fflush(model_trace_file);
    }
}

#ifndef RA3_MODEL_TRACE_JOURNAL_ONLY
#define MODEL_TRACE_EDITS 4u
static volatile uintptr_t model_trace_select_resume __attribute__((used)) = 0x8b0456u;
static volatile uintptr_t model_trace_commit_resume __attribute__((used)) = 0x8b050du;
static volatile uintptr_t model_trace_advance_resume __attribute__((used)) = 0x8b2f96u;
static volatile uintptr_t model_trace_after_resume __attribute__((used)) = 0x8b334eu;

/* Module addresses are session identities. No protected name/query getter is
 * invoked. The raw template-name word is retained without claiming a hash. */
static uint32_t model_trace_u32(const void *base, unsigned offset) {
    uint32_t value;
    memcpy(&value, (const BYTE *)base + offset, 4);
    return value;
}
static uint32_t model_trace_loco_type(void *object) {
    BYTE *owner = object ? *(BYTE **)((BYTE *)object + 0x384) : NULL;
    BYTE *loco = owner ? *(BYTE **)(owner + 0x228) : NULL;
    BYTE *link = loco ? *(BYTE **)(loco + 4) : NULL;
    BYTE *parameters = link ? *(BYTE **)(link + 4) : NULL;
    return parameters ? model_trace_u32(parameters, 0x64) : 0;
}
static void model_trace_snapshot(void *module, unsigned kind, unsigned extra) {
    BYTE *drawable = *(BYTE **)((BYTE *)module + 8);
    if (!drawable) return;
    BYTE *object = *(BYTE **)(drawable + 0x138);
    BYTE *templ = object ? *(BYTE **)(object + 4) : NULL;
    uint32_t words[20] = {
        (uint32_t)(uintptr_t)module, (uint32_t)(uintptr_t)object,
        (uint32_t)(uintptr_t)templ, templ ? model_trace_u32(templ, 4) : 0,
        model_trace_u32(module, 0xc), model_trace_u32(module, 0x3c),
        model_trace_u32(module, 0x40), model_trace_u32(module, 0x64),
        model_trace_u32(module, 0xc0), model_trace_u32(module, 0xc4),
        model_trace_u32(module, 0xc8), *(uint32_t *)0x00cf4c68u,
        model_trace_loco_type(object), model_trace_u32(drawable, 0x130),
        object ? model_trace_u32(object, 0xb4) : 0,
        object ? model_trace_u32(object, 0xb8) : 0,
        object ? model_trace_u32(object, 0xbc) : 0,
        model_trace_u32(module, 0x11c), model_trace_u32(module, 0x120), extra
    };
    model_trace_emit(kind, drawable, words, sizeof(words) / sizeof(*words));
}
typedef struct {
    void *module;
    uint32_t type, state, conditions, next_ms, burst_until, client_frame;
    int sample;
} ModelTraceSample;
static SRWLOCK model_trace_sample_lock = SRWLOCK_INIT;
static ModelTraceSample model_trace_samples[1024];
/* Five snapshots/s normally; full display cadence for two seconds after a
 * mode/condition change. The paired after-event uses the entry's decision. */
static int model_trace_sample(void *module, unsigned after) {
    BYTE *drawable = *(BYTE **)((BYTE *)module + 8);
    BYTE *object = drawable ? *(BYTE **)(drawable + 0x138) : NULL;
    uint32_t type = model_trace_loco_type(object);
    if (!type || !enabled) return 0;
    uint32_t now = GetTickCount() - model_trace_started_ms;
    BYTE *client = *(BYTE **)0x00ceeff8u;
    uint32_t frame = client ? model_trace_u32(client, 0x48) : 0;
    uint32_t state = model_trace_u32(module, 0x40);
    uint32_t conditions = model_trace_u32(object, 0xb8);
    if (!TryAcquireSRWLockExclusive(&model_trace_sample_lock)) return 0;
    ModelTraceSample *sample = &model_trace_samples[((uintptr_t)module >> 4u) & 1023u];
    if (sample->module != module) {
        *sample = (ModelTraceSample){module, type, state, conditions, 0, 0, frame, 0};
    }
    if (!after) {
        if (sample->type != type || sample->conditions != conditions || sample->state != state)
            sample->burst_until = now + 2000;
        sample->type = type;
        sample->state = state;
        sample->conditions = conditions;
        sample->client_frame = frame;
        sample->sample = now < sample->burst_until || now >= sample->next_ms;
        if (sample->sample) sample->next_ms = now + 200;
    }
    int result = sample->sample && sample->client_frame == frame;
    ReleaseSRWLockExclusive(&model_trace_sample_lock);
    return result;
}
__attribute__((used,noinline)) static void model_trace_select(void *module, uint32_t incoming) {
    if (incoming != model_trace_u32(module, 0x40))
        model_trace_snapshot(module, MODEL_TRACE_TRANSITION, incoming);
}
__attribute__((used,noinline)) static void model_trace_commit(void *module) {
    model_trace_snapshot(module, MODEL_TRACE_COMMIT, 0);
}
__attribute__((used,noinline)) static void model_trace_advance(void *module, unsigned after) {
    if (!model_trace_sample(module, after)) return;
    model_trace_snapshot(module, after ? MODEL_TRACE_CLIP_AFTER : MODEL_TRACE_CLIP, 0);
    BYTE *drawable = *(BYTE **)((BYTE *)module + 8);
    for (unsigned i = 0; drawable && i < 3; ++i) {
        BYTE *channel = (BYTE *)module + 0x11c + i * 0x1c;
        BYTE *clip = *(BYTE **)channel;
        if (!clip) continue;
        uint32_t words[] = {
            (uint32_t)(uintptr_t)module, i, after, (uint32_t)(uintptr_t)clip,
            model_trace_u32(channel, 4), model_trace_u32(channel, 8),
            model_trace_u32(channel, 12), model_trace_u32(channel, 16),
            model_trace_u32(channel, 20), channel[24],
            model_trace_u32(clip, 0x14), model_trace_u32(clip, 0x18),
            model_trace_u32(module, 0x40), model_trace_u32(module, 0x64),
            model_trace_u32(module, 0xc0), model_trace_u32(module, 0xc4),
            model_trace_u32(module, 0xc8), *(uint32_t *)0x00cf4c68u,
            ((BYTE *)module)[0xa8], ((BYTE *)module)[0x208]
        };
        model_trace_emit(MODEL_TRACE_CHANNEL, drawable, words, sizeof(words) / sizeof(*words));
    }
}

/* Entry args retain their original stack locations in the saved-ESP frame.
 * Internal hooks preserve all integer and floating-point native state. */
#define MODEL_TRACE_SAVE \
    "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\t" \
    "fxsave (%esp)\n\tsubl $16,%esp\n\t"
#define MODEL_TRACE_RESTORE \
    "addl $16,%esp\n\tfxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\t"
__attribute__((naked,used)) static void model_trace_select_bridge(void) {
    __asm__ __volatile__(MODEL_TRACE_SAVE
        "movl %ecx,(%esp)\n\tmovl 0x28(%ebp),%eax\n\tmovl %eax,4(%esp)\n\t"
        "call _model_trace_select\n\t" MODEL_TRACE_RESTORE
        "pushl %ebx\n\tpushl %ebp\n\tmovl 0xc(%esp),%ebp\n\t"
        "jmp *_model_trace_select_resume\n\t");
}
__attribute__((naked,used)) static void model_trace_commit_bridge(void) {
    __asm__ __volatile__(
        "movl %ebp,0x40(%esi)\n\tmovss %xmm0,0xc4(%esi)\n\t"
        MODEL_TRACE_SAVE "movl %esi,(%esp)\n\tcall _model_trace_commit\n\t"
        MODEL_TRACE_RESTORE "jmp *_model_trace_commit_resume\n\t");
}
__attribute__((naked,used)) static void model_trace_advance_bridge(void) {
    __asm__ __volatile__(MODEL_TRACE_SAVE
        "movl %ecx,(%esp)\n\tmovl $0,4(%esp)\n\tcall _model_trace_advance\n\t"
        MODEL_TRACE_RESTORE
        "subl $0x18,%esp\n\tpushl %esi\n\tmovl %ecx,%esi\n\t"
        "jmp *_model_trace_advance_resume\n\t");
}
__attribute__((naked,used)) static void model_trace_after_bridge(void) {
    __asm__ __volatile__(MODEL_TRACE_SAVE
        "movl %esi,(%esp)\n\tmovl $1,4(%esp)\n\tcall _model_trace_advance\n\t"
        MODEL_TRACE_RESTORE
        "cmpl %ebx,0x11c(%esi)\n\tjmp *_model_trace_after_resume\n\t");
}
static BOOL model_trace_prepare_edits(void) {
    const BYTE select[] = {0x53,0x55,0x8b,0x6c,0x24,0x0c};
    const BYTE commit[] = {0x89,0x6e,0x40,0xf3,0x0f,0x11,0x86,0xc4,0,0,0};
    const BYTE advance[] = {0x83,0xec,0x18,0x56,0x8b,0xf1};
    const BYTE after[] = {0x39,0x9e,0x1c,0x01,0,0};
    return add_jump(0x8b0450, select, sizeof(select), model_trace_select_bridge) &&
           add_jump(0x8b0502, commit, sizeof(commit), model_trace_commit_bridge) &&
           add_jump(0x8b2f90, advance, sizeof(advance), model_trace_advance_bridge) &&
           add_jump(0x8b3348, after, sizeof(after), model_trace_after_bridge);
}
#undef MODEL_TRACE_SAVE
#undef MODEL_TRACE_RESTORE
#endif
#endif
#endif
