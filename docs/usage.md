# Usage

Tool reference for building payloads, packaging releases and using the loader.
The end-user frame-rate chooser lives in the
[RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) app; this document is for
building from source.

## Building payloads and the loader

Requirements: CMake 3.15 or newer (Ninja recommended), Python 3, and an i686
Windows compiler for the payloads (MinGW-w64 GCC or LLVM-MinGW Clang).

Host tests run on any platform with a C compiler:

```bash
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Payloads and loader (i686 Windows, cross or native):

```bash
cmake -S . -B build/i686 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/i686-windows.cmake \
  -DRA3EP1FPSUNLOCK_C_COMPILER=/path/to/i686-w64-mingw32-clang \
  -DBUILD_TESTING=OFF
cmake --build build/i686
```

- `cmake/i686-windows.cmake` selects the i686 Windows toolchain;
  `RA3EP1FPSUNLOCK_C_COMPILER` overrides the compiler when its default is not
  the one you want.
- Rates are chosen with `-DRA3EP1FPSUNLOCK_RATES="60;120"` (the default).
  Entries must be 30, 60, 90 or 120; 30 and 90 are research switches of the
  same source tree, not release profiles.
- Each configured rate builds `fps<rate>.dll` from the shared source; the
  loader builds as `RA3EP1FPSUnlock.exe`. There is no runtime rate switch.

## Release archive

`tools/release.py` writes a deterministic zip from a build directory:

```bash
python3 tools/release.py --build-dir build/i686 \
  --output dist/RA3EP1-FPS-Unlock-v1.0.0-windows-x86.zip
```

The archive contains `fps<rate>.dll` for each configured rate, the loader as
`RA3EP1FPSUnlock.exe` plus an identical copy named `Uprising60.exe`, `LICENSE`
and `README.md` from the repository root, and a `SHA256SUMS` file. Zip entries
carry fixed metadata and are written in sorted order, so identical inputs
produce byte-identical archives. `python3 tools/release.py --help` documents
the remaining options.

## Using the loader

```text
RA3EP1FPSUnlock.exe GAME_DIRECTORY DLL_PATH
```

- `GAME_DIRECTORY` is the Uprising game root directory. The loader verifies
  `Data/ra3ep1_1.0.game` by SHA-256, refuses to start any other executable,
  and never writes the game executable.
- `DLL_PATH` is a payload DLL built by this project.
- Payload diagnostics are written to `uprising120.log` next to the loaded
  DLL.
- Setting `U60_BASELINE` to a non-empty value keeps native timing and logs
  baseline-only measurements.

## Research and diagnostic switches

All compile-time, and none are release profiles:

- `RA3_TARGET_FPS=30` / `RA3_TARGET_FPS=90` — research rate targets.
- `RA3_DIAGNOSTIC_NATIVE_SWAY=1` — keeps the unmodified native suspension
  path, used to isolate whether a symptom requires the suspension wrapper.
- `RA3_DIAGNOSTIC_UNIT_WINDOW=10` — uses the 120 FPS age window at 90 FPS.
- `RA3_MODEL_TRACE=1` — enables the bounded model-state and clip-progress
  journal in `src/model_trace.h`; it defaults to 0 and stable binaries
  compile it out entirely.

## Reading results

- `SHA256SUMS` — SHA-256 of every archive member.
- CTest output — the per-rate host suites; the bridge fixture prints
  `PASS: actual i686 suspension` and exits 42 on success.
- Session telemetry from live verification lives with the owning project's
  recorded sessions; see [testing.md](testing.md) for what each stage proves.
