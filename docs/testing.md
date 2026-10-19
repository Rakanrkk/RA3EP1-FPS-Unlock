# Testing

What is tested, at which stage, and what the results do and do not claim.

## Host tests (per rate)

CTest builds each suite once per configured rate:

| Suite | Covers |
| --- | --- |
| `tests/test_schedule120.c` | Stage endpoints per rate, exact native-stage counts, first-cycle alignment, hold-slot placement, millisecond accumulation |
| `tests/test_particle_clock.c` | Authored 30 Hz cadence, frames-per-step per rate, client-domain scale |
| `tests/test_sway_compat.c` | Suspension clock math: sample cadence, interpolation phases, first-sample seeding |

## Contract check

`tests/contract_check.py` is an independent behavioral verifier: it reads the
headers as data, compiles a small host driver that executes the actual header
functions at each `RA3_TARGET_FPS`, and compares observed behavior against
expectations derived independently in the script (exact stage-ending tables,
fractional millisecond clocks, float32 bit patterns). The frozen ground
truths are the accepted 120 schedule (`1,2,3,3,4,5,5,6` with holds re-display
stages 3 and 5), the accepted 60 batching (`2,3,5,6`) and the native stock
30 timing.

## Real 32-bit bridge fixture

`tests/test_sway_runtime.c` builds and runs the actual emitted i686 bridges in
a real 32-bit process:

- generated entry opcodes for every guard site;
- allocator and call ABI, including a native-sized state ending on an
  inaccessible page;
- all eight native dispatch cases with independent stand-in path writers;
- cache invalidation on locomotor/type/ownership change and at the dispatcher
  entry;
- fresh caller poses on cached displays and no-output invalidation;
- flags, general-register and x87/SSE preservation across output hooks.

The runner verifies the original executable's SHA-256 and all native table
dwords before running the owned fixture. The fixture runs in-place only when
the build itself targets i686 Windows; CTest gates it on its `PASS` banner
(exit 42 on success). These fixtures exercise wrapper behavior against
selected native contracts; they do not execute the complete native physics
functions or prove every dynamic native branch.

## Live sessions

Every gameplay claim traces to a recorded session: the exact DLL (by
SHA-256), raw per-window telemetry, and the player's visual report. Steady
windows are reported alongside loading and low-clock windows rather than
averaged into a single number.

## Scope limits

- Windows-native execution of the loader and payloads is not asserted by this
  repository; CI compiles the same sources with MinGW-w64 and runs the host
  tests.
- Host tests and fixtures do not execute complete native physics functions.
- Telemetry measures hooks, not GPU Present.
- Visual acceptance covers the tested session's content, hardware and
  duration; it is not extended to other machines or all game content.
- Diagnostic builds are never used as acceptance evidence.

Per-rate status: see the table in [the README](../README.md#release-status)
and the open issue in
[research/90fps-landing.md](research/90fps-landing.md).
