# Open investigation: moving-landing transformation (90 FPS)

Status: **open**. Recorded here so the next step starts from evidence, not
guesswork.

## Symptom

When a Jet Tengu lands while still moving (jet form to ground form), the
transformation does not always play through cleanly at high rates: the
transformation animation appears to lose or delay frames and the final ground
form snaps in. Stationary landings play normally. The symptom was reported
during 90 and 120 FPS testing; the 120 FPS acceptance covered the tested
sessions and is not extended to this scene.

An earlier high-frequency model shake after moving landings at 90 FPS was
reported as no longer observed after the corrected-dispatch revision.

## What has been tested and ruled out

Each hypothesis below was isolated with a diagnostic build and is retained in
the session archive:

- **Suspension wrapper involvement** — with all fourteen suspension edits
  disabled (native suspension restored, scheduler/particles unchanged), the
  snap remained. The remaining symptom does not require the suspension
  wrapper.
- **Interpolation age window** — widening the three drawable age guards from
  the 90 FPS window (8) to the 120 FPS window (10) changed nothing. This
  specific hypothesis was rejected, and no further blind widening was done.
- **Suspension cache mapping and output ownership** — the corrected type
  mapping (1/7→A, 2/3/4→B, 5/6 none, 8→C), per-path output recording, and
  cache invalidation fixes were verified by 32-bit fixtures at 60/90/120.
  They fixed the reported shake, but the landing snap remained in each
  retest.
- **Paths examined and left unchanged** — the suspected four-frame
  transition turned out to be an RGB flash envelope; the examined
  millisecond-path function is client initialization, not the animation
  clock; and the proposed skeletal-clock slowdown at `0x5335c0` uses absolute
  frame time, so an additive slowdown there is not supported.

## Current direction

Instrumented model/animation telemetry (state requests, state stores, actual
clip progress) is being compared between stationary and moving landings with
identical payloads, to locate where the moving case diverges in the
transformation's animation-state progression. No color-flash or global
1/30-constant changes are made without specific evidence.

## Evidence

Observation journals, exact DLL hashes and raw logs for every diagnostic
session are retained with their source pins; the sessions and conclusions
above reference those artifacts. The corrected stationary/moving distinction
supersedes the earlier unqualified "landing normal" report, which applied
only to stationary landings.
