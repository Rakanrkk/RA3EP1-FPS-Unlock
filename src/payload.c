/* RA3EP1 FPS Unlock portable timing payload for one verified Uprising build.
 * No code or resources from Ra3FrameLab are linked into this DLL.
 * Original game files are never written. All edits die with the process.
 *
 * Copied original project source. The model-trace diagnostic journal stays
 * available behind RA3_MODEL_TRACE (default 0: fully compiled out of the
 * stable profile).
 */
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "schedule120.h"
#include "particle_clock.h"

#ifndef RA3_DIAGNOSTIC_NATIVE_SWAY
#define RA3_DIAGNOSTIC_NATIVE_SWAY 0
#endif

static HMODULE self_module;
static FILE *log_file;
static volatile LONG display_frames, enabled;
static BYTE *volatile frame_context;
static unsigned remainder_ms;
static volatile unsigned frame_ms = RA3_NOMINAL_FRAME_MS;
static unsigned effect_fps = 30;
static float effect_frame_scale = 0.03f;
static Schedule120 schedule120;
static volatile unsigned hold_display __attribute__((used));
static uintptr_t hold_continue __attribute__((used)) = 0x60e41f;
static unsigned fps = RA3_TARGET_FPS;
static float seconds_per_frame = 1.0f / RA3_TARGET_FPS;
static uintptr_t gate_continue __attribute__((used)) = 0x60e307;
static uintptr_t stage_continue __attribute__((used)) = 0x60e321;
static uintptr_t tail_continue __attribute__((used)) = 0x60e425;
static uintptr_t particle_continue __attribute__((used)) = 0x6d2860;
static unsigned particle_phase;
static volatile unsigned run_particle __attribute__((used));
static volatile LONG particle_calls, particle_steps;

static void log_line(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fputc('\n', log_file);
    fflush(log_file);
}

/* Native stage calls per 15 Hz logic cycle; middle slots interpolate. */
__attribute__((used,noinline)) static void frame_begin(BYTE *context) {
    frame_context = context;
    display_frames++;
    hold_display = 0;
    if (enabled) {
        unsigned stage;
        memcpy(&stage, context + 0x58, 4);
#if RA3_CORE_HOLD_SCHEDULER
        hold_display = schedule120_begin(&schedule120, stage);
#else
        (void)stage;
#endif
        frame_ms = next_milliseconds(&remainder_ms);
    }
}

__attribute__((used,noinline)) static void frame_end(BYTE *context) {
    if (!enabled) return;
    unsigned stage;
    memcpy(&stage, context + 0x58, 4);
    if (stage < 1 || stage > 6) return;
#if RA3_CORE_HOLD_SCHEDULER
    if (!schedule120.synchronized || !schedule120.slot) return;
    if (stage != schedule120_end(schedule120.slot)) {
        schedule120.synchronized = 0;
        schedule120.slot = 0;
        return;
    }
    unsigned bits = interpolation_bits(schedule120.slot);
#else
    unsigned bits = interpolation_bits(stage);
#endif
    memcpy(context + 0x60, &bits, 4);
}

#if RA3_TARGET_FPS == 60
static volatile unsigned computed_end __attribute__((used));
static uintptr_t batch_continue __attribute__((used)) = 0x621aec;
__attribute__((used,noinline)) static void schedule_stage(unsigned stage) {
    computed_end = end_stage(stage);
}
__attribute__((naked)) static void batch_bridge(void) {
    __asm__ __volatile__(
        "pushfl\n\tpushal\n\tpushl %esi\n\tcall _schedule_stage\n\taddl $4, %esp\n\tpopal\n\tpopfl\n\t"
        "movl _computed_end, %eax\n\tjmp *_batch_continue\n\t");
}
#endif

__attribute__((naked)) static void gate_bridge(void) {
    __asm__ __volatile__(
        "pushfl\n\tpushal\n\tpushl %esi\n\tcall _frame_begin\n\taddl $4, %esp\n\tpopal\n\tpopfl\n\t"
        "cmpl $0, _hold_display\n\tjne 2f\n\t"
        "movl 0x58(%esi), %ecx\n\tcmpl $6, %ecx\n\tjne 1f\n\t"
        "jmp *_gate_continue\n\t"
        "1: jmp *_stage_continue\n\t"
        "2: pushl %edi\n\tjmp *_hold_continue\n\t");
}

__attribute__((naked)) static void tail_bridge(void) {
    __asm__ __volatile__(
        "pushfl\n\tpushal\n\tpushl %esi\n\tcall _frame_end\n\taddl $4, %esp\n\tpopal\n\tpopfl\n\t"
        "movl 0xcea51c, %ecx\n\tjmp *_tail_continue\n\t");
}

__attribute__((used,noinline)) static void particle_tick(void) {
    particle_calls++;
    run_particle = particle_clock_step(&particle_phase);
    if (run_particle) particle_steps++;
}

/* Called from the native manager with ECX=this and no stack arguments.
 * Tail-call only its simulation step; draw bucket rebuilds after the call stay
 * in the original per-frame caller. Preserve entry state on the skipped path. */
__attribute__((naked)) static void particle_bridge(void) {
    __asm__ __volatile__(
        "pushfl\n\tpushal\n\tcall _particle_tick\n\t"
        "cmpl $0, _run_particle\n\tje 1f\n\t"
        "popal\n\tpopfl\n\tjmp *_particle_continue\n\t"
        "1: popal\n\tpopfl\n\tret\n\t");
}

typedef struct { BYTE *address; BYTE before[16], after[16]; SIZE_T size; } Edit;
static Edit edits[40];
static unsigned edit_count;

static BOOL add_edit(uintptr_t address, const void *expected, const void *replacement, SIZE_T n) {
    if (n > 16 || edit_count >= 40 || memcmp((void *)address, expected, n)) {
        log_line("REFUSED: instruction/value mismatch at %08lx", (unsigned long)address);
        return FALSE;
    }
    Edit *e = &edits[edit_count++];
    e->address = (BYTE *)address; e->size = n;
    memcpy(e->before, expected, n); memcpy(e->after, replacement, n);
    return TRUE;
}

static BOOL redirect(uintptr_t site, unsigned operand, const BYTE *expected,
                     unsigned length, const void *new_address) {
    BYTE bytes[16];
    if (length > sizeof(bytes) || operand + 4 > length) return FALSE;
    memcpy(bytes, expected, length);
    uint32_t pointer = (uint32_t)(uintptr_t)new_address;
    memcpy(bytes + operand, &pointer, 4);
    return add_edit(site, expected, bytes, length);
}

static BOOL write_memory(Edit *e, BOOL restore) {
    DWORD old_protection, ignored;
    if (!VirtualProtect(e->address, e->size, PAGE_EXECUTE_READWRITE, &old_protection)) return FALSE;
    memcpy(e->address, restore ? e->before : e->after, e->size);
    FlushInstructionCache(GetCurrentProcess(), e->address, e->size);
    BOOL ok = VirtualProtect(e->address, e->size, old_protection, &ignored);
    return ok && !memcmp(e->address, restore ? e->before : e->after, e->size);
}

/* Threads belong exclusively to this test game. Never suspend other processes. */
static BOOL commit_edits(void) {
    HANDLE held[128], snapshot;
    unsigned count = 0, done = 0;
    BOOL ok = TRUE;
    THREADENTRY32 t = { .dwSize = sizeof(t) };
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return FALSE;
    if (!Thread32First(snapshot, &t)) { CloseHandle(snapshot); return FALSE; }
    do {
        if (t.th32OwnerProcessID != GetCurrentProcessId() || t.th32ThreadID == GetCurrentThreadId()) continue;
        if (count == 128) { ok = FALSE; break; }
        HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, t.th32ThreadID);
        if (!h || SuspendThread(h) == (DWORD)-1) {
            if (h) CloseHandle(h);
            ok = FALSE; break;
        }
        held[count++] = h;
        CONTEXT context = { .ContextFlags = CONTEXT_CONTROL };
        if (!GetThreadContext(h, &context)) { ok = FALSE; break; }
        for (unsigned i = 0; i < edit_count; ++i) {
            uintptr_t start = (uintptr_t)edits[i].address;
            if (context.Eip >= start && context.Eip < start + edits[i].size) {
                ok = FALSE; break;
            }
        }
        if (!ok) break;
    } while (Thread32Next(snapshot, &t));
    if (ok) {
        for (unsigned i = 0; i < edit_count; i++) {
            if (memcmp(edits[i].address, edits[i].before, edits[i].size)) { ok = FALSE; break; }
        }
    }
    if (ok) {
        for (unsigned i = 0; i < edit_count; i++) {
            done = i + 1;
            if (!write_memory(&edits[i], FALSE)) { ok = FALSE; break; }
        }
    }
    if (!ok) while (done) {
        --done;
        if (!write_memory(&edits[done], TRUE)) TerminateProcess(GetCurrentProcess(), 91);
    }
    while (count) { --count; ResumeThread(held[count]); CloseHandle(held[count]); }
    CloseHandle(snapshot);
    return ok;
}

static BOOL add_jump(uintptr_t site, const BYTE *expected, unsigned length, void *bridge) {
    BYTE bytes[16];
    if (length < 5 || length > sizeof(bytes)) return FALSE;
    memset(bytes, 0x90, length); bytes[0] = 0xe9;
    int32_t offset = (int32_t)((uintptr_t)bridge - site - 5);
    memcpy(bytes + 1, &offset, 4);
    return add_edit(site, expected, bytes, length);
}

static BOOL add_call(uintptr_t site, const BYTE *expected, void *bridge) {
    BYTE bytes[5] = {0xe8, 0, 0, 0, 0};
    int32_t offset = (int32_t)((uintptr_t)bridge - site - 5);
    memcpy(bytes + 1, &offset, 4);
    return add_edit(site, expected, bytes, sizeof(bytes));
}

#include "sway_compat.h"
#include "model_trace.h"

static BOOL install_counter(void) {
    const BYTE gate[] = {0x8b,0x4e,0x58,0x83,0xf9,0x06,0x75,0x1a};
    const BYTE tail[] = {0x8b,0x0d,0x1c,0xa5,0xce,0x00};
    if (!add_jump(0x60e2ff,gate,sizeof(gate),gate_bridge) ||
        !add_jump(0x60e41f,tail,sizeof(tail),tail_bridge)) return FALSE;
#if RA3_MODEL_TRACE
    if (!model_trace_prepare_edits()) return FALSE;
#endif
    return commit_edits();
}

static BOOL install_120(void) {
    edit_count = 0;
    if (!RA3_DIAGNOSTIC_NATIVE_SWAY && !sway_prepare_edits()) return FALSE;
#if RA3_TARGET_FPS == 60
    const BYTE batch[] = {0x8d,0x4e,0xff,0x0f,0xaf,0xcf};
    unsigned original_threshold = 6, threshold = 8;
    if (!add_jump(0x621ace,batch,sizeof(batch),batch_bridge) ||
        !add_edit(0x60b606,&original_threshold,&threshold,4)) return FALSE;
#endif
    const BYTE particle_call[] = {0xe8, 0xee, 0x28, 0x0d, 0x00};
    if (!add_call(0x5fff6d, particle_call, particle_bridge)) return FALSE;
    /* This age gate is separate from the engine's interpolation fraction. */
#if RA3_UNIT_WINDOW != 6
    const BYTE age_guard[] = {0x2b, 0x86, 0x30, 0x01, 0x00, 0x00, 0x83, 0xf8, 0x06};
    BYTE widened_guard[sizeof(age_guard)];
    memcpy(widened_guard, age_guard, sizeof(age_guard));
    widened_guard[8] = RA3_UNIT_WINDOW;
    const uintptr_t age_sites[] = {0x53065a, 0x535e67, 0x535f50};
    for (unsigned i = 0; i < sizeof(age_sites) / sizeof(*age_sites); ++i) {
        if (!add_edit(age_sites[i], age_guard, widened_guard, sizeof(age_guard))) return FALSE;
    }
#endif
    unsigned original_fps = 30;
    if (!add_edit(0xcc3a9c,&original_fps,&fps,4)) return FALSE;
    /* Camera step-count conversions use the native integer nominal frame period. */
    unsigned original_ms = 33, nominal_ms = RA3_NOMINAL_FRAME_MS;
    if (!add_edit(0xcf506c,&original_ms,&nominal_ms,4)) return FALSE;
    /* Actual client/display frame counter remains 120 Hz. GPU particle resources
     * still use authored 30-Hz lifetime units, including their expiry check. */
    const BYTE particle_scale[] = {0xd8,0x0d,0x0c,0xf5,0xce,0};
    const BYTE expiry[] = {0x0f,0xaf,0x05,0x9c,0x3a,0xcc,0};
    if (!redirect(0x6c1b5a,2,particle_scale,sizeof(particle_scale),&effect_frame_scale) ||
        !redirect(0x6ce6d9,3,expiry,sizeof(expiry),&effect_fps)) return FALSE;
    const BYTE clock1[] = {0x03,0x05,0x6c,0x50,0xcf,0};
    const BYTE clock2[] = {0x8b,0x0d,0x6c,0x50,0xcf,0};
    if (!redirect(0x60a7d2,2,clock1,sizeof(clock1),(const void *)&frame_ms) ||
        !redirect(0x60a811,2,clock2,sizeof(clock2),(const void *)&frame_ms)) return FALSE;
    const BYTE visual[] = {0xf3,0x0f,0x58,0x05,0xac,0x57,0xbf,0};
    if (!redirect(0x895edc,4,visual,sizeof(visual),&seconds_per_frame)) return FALSE;
    /* General client conversions follow 120 Hz; only GPU particle units stay .03. */
    float effect_scale = *(float *)0xcef50c;
    if (effect_scale < 0.029999f || effect_scale > 0.030001f) {
        log_line("REFUSED: unexpected authored effect time scale %.9f",effect_scale);
        return FALSE;
    }
    const uintptr_t derived[] = {0xcef500,0xcef504,0xcef5e8,0xcef50c};
    const float before[] = {30.0f,1000.0f/30.0f,1.0f/30.0f,0.03f};
    const float after[] = {(float)RA3_TARGET_FPS,1000.0f/RA3_TARGET_FPS,1.0f/RA3_TARGET_FPS,RA3_EFFECT_CLIENT_SCALE};
    for (unsigned i=0; i<sizeof(derived)/sizeof(*derived); i++) {
        float current = *(float *)derived[i];
        if (current < before[i]*0.9999f || current > before[i]*1.0001f) {
            log_line("REFUSED: derived float %08lx = %.9f",(unsigned long)derived[i],current); return FALSE;
        }
        if (!add_edit(derived[i],&current,&after[i],4)) return FALSE;
    }
    unsigned original_ratio=2, new_ratio=RA3_DISPLAY_RATIO;
    if (!frame_context || !add_edit((uintptr_t)(frame_context+0x5c), &original_ratio,&new_ratio,4)) return FALSE;
    unsigned expected_edits = RA3_EXPECTED_CORE_EDITS - (RA3_DIAGNOSTIC_NATIVE_SWAY ? 14u : 0u);
    if (edit_count != expected_edits) {
        log_line("REFUSED: incomplete %u FPS edit set (%u instead of %u)",
                 fps, edit_count, expected_edits);
        return FALSE;
    }
    remainder_ms = 0;
    frame_ms = RA3_NOMINAL_FRAME_MS;
    schedule120.slot = 0;
    schedule120.synchronized = 0;
    particle_phase = 0;
    run_particle = 0;
    particle_calls = particle_steps = 0;
    if (!commit_edits()) return FALSE;
    enabled = 1;
    return TRUE;
}

static int read_counter(uintptr_t global, unsigned offset) {
    uintptr_t p = *(uintptr_t *)global;
    MEMORY_BASIC_INFORMATION m;
    if (!p || !VirtualQuery((void *)(p+offset), &m,sizeof(m)) || m.State != MEM_COMMIT ||
        (m.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return -1;
    return *(int *)(p+offset);
}

static void measure(const char *label, unsigned seconds) {
    LARGE_INTEGER frequency,start,end;
    QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&start);
    int client0=read_counter(0xceeff8,0x48);
    LONG f0=display_frames, p0=particle_calls, s0=particle_steps;
    int l0=read_counter(0xcec568,0x50); unsigned c0=*(unsigned *)0xcf4c68;
#if RA3_MODEL_TRACE
    for (unsigned tick = 0; tick < seconds * 10; ++tick) {
        Sleep(100);
        model_trace_flush();
    }
    model_trace_counters();
#else
    Sleep(seconds*1000);
#endif
    QueryPerformanceCounter(&end);
    double elapsed=(double)(end.QuadPart-start.QuadPart)/frequency.QuadPart;
    int l1=read_counter(0xcec568,0x50); unsigned c1=*(unsigned *)0xcf4c68;
    int client1=read_counter(0xceeff8,0x48);
    log_line("%s dt=%.3f render_updates=%.3f logic=%.3f logic_frames=%d..%d clock_rate=%.4f frame_ms=%u globals=%u/%u client_hz=%.3f client_frames=%d..%d particle_calls_hz=%.3f particle_steps_hz=%.3f",
        label,elapsed,(display_frames-f0)/elapsed,(l0>=0&&l1>=l0)?(l1-l0)/elapsed:-1.0,
        l0,l1,c1>=c0?(c1-c0)/1000.0/elapsed:-1.0,frame_ms,*(unsigned *)0xcc3a9c,*(unsigned *)0xcc3a98,
        (client0>=0&&client1>=client0)?(client1-client0)/elapsed:-1.0,client0,client1,
        (particle_calls-p0)/elapsed,(particle_steps-s0)/elapsed);
}

static DWORD WINAPI worker(void *unused) {
    (void)unused;
    wchar_t path[MAX_PATH]; GetModuleFileNameW(self_module,path,MAX_PATH);
    wchar_t *slash=wcsrchr(path,L'\\'); if (!slash) return 1;
    wcscpy(slash+1,L"uprising120.log");
    log_file=_wfopen(path,L"w"); if (!log_file) return 2;
    log_line("RA3EP1 FPS Unlock %u FPS payload (30 Hz suspension and particles); pid=%lu",fps,GetCurrentProcessId());
    if ((uintptr_t)GetModuleHandleW(NULL)!=0x400000 ) {
        log_line("REFUSED: unexpected base"); fclose(log_file); return 3;
    }
    wchar_t image_path[MAX_PATH];
    GetModuleFileNameW(NULL,image_path,MAX_PATH);
    const wchar_t *name=wcsrchr(image_path,L'\\'); name=name?name+1:image_path;
    if (_wcsicmp(name,L"ra3ep1_1.0.game")) {
        log_line("REFUSED: not the supported game executable; no game memory read or edited");
        fclose(log_file); return 7;
    }
    const IMAGE_DOS_HEADER *dos=(const IMAGE_DOS_HEADER *)GetModuleHandleW(NULL);
    const IMAGE_NT_HEADERS32 *pe=(const IMAGE_NT_HEADERS32 *)((const BYTE *)dos+dos->e_lfanew);
    if (pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
        pe->FileHeader.TimeDateStamp!=0x49a7778c || pe->OptionalHeader.SizeOfImage!=0x2791f38) {
        log_line("REFUSED: unsupported PE build; no game memory read or edited");
        fclose(log_file); return 8;
    }
    for (unsigned i=0;i<300;i++) {
        if (*(unsigned *)0xcc3a9c==30 && *(unsigned *)0xcc3a98==15 && *(unsigned *)0xcf506c==33 && *(float *)0xcef500==30.0f) break;
        Sleep(100);
    }
    if (*(unsigned *)0xcf506c!=33 || *(unsigned *)0xcc3a98!=15) {
        log_line("REFUSED: engine not initialized"); fclose(log_file); return 4;
    }
#if RA3_MODEL_TRACE
    wcscpy(slash+1,L"uprising-model.trace");
    if (!model_trace_open(path)) {
        log_line("REFUSED: diagnostic model journal could not be opened");
        fclose(log_file); return 10;
    }
    log_line("Diagnostic model trace: four native animation hooks; bounded 64 MiB journal; no animation parameter edits");
#endif
    if (!install_counter()) {log_line("REFUSED: counter hook mismatch or write failed"); fclose(log_file); return 5;}
    log_line("Counter installed; waiting for first rendered frames");
    for (unsigned i=0;i<1200 && display_frames<30;i++) Sleep(100);
    if (display_frames<30) {log_line("REFUSED: no active render loop within 120 seconds");fclose(log_file);return 9;}
    log_line("Measuring 30 FPS baseline before changes");
    measure("BASELINE",5);
    char baseline[8];
    if (RA3_CORE_DISABLED || GetEnvironmentVariableA("U60_BASELINE",baseline,sizeof(baseline))) {
        log_line("Baseline-only mode: no timing changes");
    } else {
        if (!install_120()) {log_line("REFUSED: core patch not committed"); fclose(log_file); return 6;}
        log_line("Core installed: target %u updates/s / 15 Hz, %u display slots / 6 native stages",fps,(unsigned)RA3_DISPLAY_RATIO);
        log_line("Authored clocks: particle simulation 30/%u, unit interpolation age window %u; draw buckets every frame",fps,(unsigned)RA3_UNIT_WINDOW);
        if (RA3_DIAGNOSTIC_NATIVE_SWAY)
            log_line("DIAGNOSTIC: native suspension unchanged; no suspension hooks/cache/rate correction installed");
        else
            log_line("Suspension installed: 30 Hz physics; actual native output masks; external cache; native state/save layout unchanged");
        log_line("Time domains: client_hz=%u client_frames_per_ms=%.6f GPU_frames_per_ms=%.6f GPU_expiry_hz=%u",*(unsigned *)0xcc3a9c,*(float *)0xcef50c,effect_frame_scale,effect_fps);
    }
    for (unsigned i=0;i<720;i++) measure(enabled?"ACTIVE":"BASELINE",5);
    fclose(log_file); return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if(reason==DLL_PROCESS_ATTACH) {
        self_module=instance; DisableThreadLibraryCalls(instance);
        HANDLE thread=CreateThread(NULL,0,worker,NULL,0,NULL);
        if(thread) CloseHandle(thread);
    }
    return TRUE;
}
