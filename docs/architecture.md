# Architecture

This document describes the timing model of the payloads: which clocks stay
native, which are re-scheduled per rate, and where interpolation bridges the
difference.

## Fixed clocks

| Clock | Rate | Notes |
| --- | --- | --- |
| Game logic | 15 Hz | Untouched at every selectable rate. No option changes simulation speed. |
| CPU particle simulation | 30 Hz | Authored cadence preserved; forwarded once per 2/4/6/8 client frames at 30/60/90/120. |
| Suspension physics | 30 Hz | Spring physics sample at the authored cadence; the display interpolates poses between samples. |
| GPU particle lifetimes | authored units | The authored 30 Hz lifetime units (0.03) stay; only the client-domain scale changes per rate. |

## Per-rate schedules

The native engine distributes the six logic stages of a 15 Hz cycle over
client display slots. One source tree builds one DLL per rate
(`RA3_TARGET_FPS`); there is no runtime rate switch.

| Rate | Display slots per cycle | Native stage endpoints | Client step | Particle step every | Sway interpolation | Age window |
| --- | --- | --- | --- | --- | --- | --- |
| 30 | 2 | native (no core timing edits) | native | frame | none (native) | 6 |
| 60 | 4 | 2/3/5/6 (native 4:1 batching) | native | 4 frames | 2 display steps | 6 |
| 90 | 6 | 1/2/3/4/5/6 | ~11/12 ms | 6 frames | 3 display steps | 8 |
| 120 | 8 | 1/2/3/3/4/5/5/6 (holds at stages 3 and 5) | 8/8/9 ms fractional | 8 frames | 4 display steps | 10 |

Notes:

- The 120 schedule holds two slots per cycle: two of the eight display frames
  interpolate without advancing a native stage, keeping stage counts exact.
  This is why raising a single FPS constant was never sufficient.
- The client step is fractional at 120 (8/8/9 ms) and 90 (11/12 ms) so the
  cycle length stays exact; authored GPU particle time units remain separate
  from client-frame units.
- The 30 build instruments counters only; it proves the telemetry pipeline
  and keeps native timing, rather than approximating a higher rate.

## Suspension pipeline

Native suspension has three output paths behind a type dispatcher. The
payloads wrap the dispatcher and the path functions:

| Native type | Path | Function |
| --- | --- | --- |
| 1, 7 | A | `0x52e4d0` |
| 2, 3, 4 | B | `0x526930` |
| 5, 6 | (no suspension output) | — |
| 8 | C | `0x52dbd0` |

The wrapper preserves fresh caller-owned fields, captures native-owned outputs
per call, and interpolates the pose across the display steps listed above. The
first sample of a cycle is seeded, never interpolated from zero. Cache entries
live in DLL-owned storage keyed by native state and drawable, and are
invalidated on locomotor, type or ownership change and before native early
exits at the dispatcher entry. Native state remains 0x70 bytes; the save
layout is unchanged.

Path A's runtime output shape is read from the function's actual selection at
`0x52e5f3`, so full and short output paths are recorded independently; output
hooks preserve flags, general registers and x87/SSE state and replay each
displaced instruction.

## Effects and particles

- CPU simulation: the particle simulation call is forwarded once per N client
  frames (N from the table above); the caller's draw-bucket rebuild continues
  every frame.
- GPU lifetimes: authored 30 Hz units are kept; the client-domain scale that
  replaces the authored `0.03` constant scales with the display rate
  (0.03/0.06/0.09/0.12).

## Interpolation age guards

Three per-drawable guards compare a client-frame age against a drawable stamp
to decide interpolation eligibility. The window widens with the cycle length
(6/6/8/10 frames at 30/60/90/120). These are runtime comparison windows only;
the engine alpha at `context+0x60` is not modified, and widening the window
alone cannot extend eligibility elsewhere.

## Runtime-only guarantee

All edits apply to runtime code paths guarded by exact byte signatures. Save
layout, replay data, and native physics-state bytes are unchanged, so saves
and replays move across rates without conversion.

## Build identity

Every release archive carries a `SHA256SUMS` file binding each artifact to its
bytes, and each build pins its inputs through the CMake build and
`tools/release.py`. Consumers (for example the RA3EP1Mac launcher) verify
these identities before use and stage a session-local copy of the selected
payload.
