# Methods

How findings are made and how claims are checked in this project.

## First-hand binary evidence

- Analysis starts from the shipped, SHA-256-pinned game executable
  (`ra3ep1_1.0.game`). Static conclusions come from the binary's own bytes:
  jump tables are decoded from all of their dwords, call targets are resolved
  from the instruction stream, and candidate interpretations are checked
  against runtime traces before use.
- Every timing edit is installed only where the exact native byte sequence
  matches, so a payload cannot silently apply to a different executable or a
  shifted layout.

## External references

Published community research is used to suggest where to look, not what to
conclude. [SAGE-Unlocked](https://github.com/TheeHorse/SAGE-Unlocked)
(GPL-3.0) identified the interpolation-window and particle-update bug classes
and was valuable for targeting; its published RA3 addresses are not assumed to
apply to the Uprising executable, and none of its code is used here. Sites
located with its patterns are re-derived from this binary's disassembly and
verified against runtime behavior. Pattern provenance is recorded per finding
in the research notes.

## Verification stages

Claims are tied to the weakest stage that supports them:

1. **Host unit tests** — the scheduling, particle-clock and suspension-clock
   math run as native host tests per target rate (exact stage counts, cycle
   alignment, fractional milliseconds, interpolation phases).
2. **Independent contract check and real 32-bit fixtures** — an independent
   verifier executes the actual header functions per rate against a derived
   model, and the emitted i686 code runs in a real 32-bit process against
   owned stand-ins for the native functions: generated entry opcodes,
   allocator and call ABI, cache invalidation, all eight dispatch cases, and
   register/state preservation across output hooks.
3. **Live game sessions** — isolated sessions with telemetry windows over
   render/update hooks, logic counters, the game clock and client-frame
   counters, plus the player's visual report.

Stages 1 and 2 do not execute the complete native physics functions and do not
prove every dynamic branch; stage 3 telemetry measures hooks, not GPU Present.
Visual acceptance from a session covers that session's content and duration.
These limits are stated rather than averaged away.

## Evidence retention

Each session keeps its raw logs, the exact DLL (SHA-256) and the source pins
it ran with, so any recorded observation can be re-attached to an exact build.
Diagnostic builds are labeled as diagnostics in their reports and are not used
as acceptance evidence.
