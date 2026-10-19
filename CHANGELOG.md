# Changelog

## 1.0.0

First release of the generalized timing payload.

- One source tree builds one payload DLL per target rate; the native 15 Hz
  logic clock is preserved at every rate and no rate changes simulation
  speed. Stable releases build 60 and 120; 30 and 90 remain research-only
  compile-time switches.
- Per-rate schedules: 30 keeps native engine timing (instrumented build
  only); 60 uses the native 4-slot batching plus corrected effect/suspension
  time domains; 90 distributes the six native stages over six display slots;
  120 distributes them over eight slots with two hold bridges.
- Particles keep the authored 30 Hz simulation and lifetime units at every
  rate; suspension physics keeps the authored 30 Hz cadence with display-side
  pose interpolation (2/3/4 display steps per cycle at 60/90/120).
- Interpolation age guards are runtime-only changes; save layout, replay data
  and native physics-state bytes are untouched.
- Portable CMake build with per-rate host suites, an independent behavioral
  contract check and a real i686 bridge fixture; `tools/release.py` writes a
  deterministic release archive including the `RA3EP1FPSUnlock.exe` loader
  (an identical copy ships as `Uprising60.exe`).
- Payload diagnostics go to `uprising120.log`; `U60_BASELINE` keeps native
  timing for baseline-only measurement.
- Research notes included, including the open Jet Tengu moving-landing
  investigation at high rates (see `docs/research/90fps-landing.md`).

Tested environment: real game sessions on macOS (Apple Silicon, via the
[RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) project). The 60 and 120
rates have local visual acceptance; the 90-rate moving-landing issue above
remains open.
