# RA3EP1 FPS Unlock

High frame-rate timing patches for Command & Conquer: Red Alert 3 – Uprising (executable `ra3ep1_1.0.game`). Each payload decouples the client update rate from the engine's fixed 15 Hz logic clock and corrects effect, particle and suspension timing for its rate. 中文说明见 [README.zh-CN.md](README.zh-CN.md)。

This is an unofficial compatibility project and is not affiliated with or endorsed by EA. Component sources and licensing: [NOTICE.md](NOTICE.md), [THIRD-PARTY.md](THIRD-PARTY.md), [docs/provenance.md](docs/provenance.md).

The project is independent of the macOS port. These patches have been verified in real game sessions on macOS (Apple Silicon, Wine/WoW64 + mtld3d) through the separately developed [RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) project; that Mac wrapper is recorded here as the tested environment and is not an application example of this one.

## How it works

The Uprising engine simulates game logic on a fixed 15 Hz cycle, and the stock game couples client updates to that clock. Each payload decouples the two: display updates run at the selected rate while the 15 Hz logic schedule, the authored 30 Hz particle simulation and the authored 30 Hz suspension physics keep their original cadence. Display-side interpolation bridges the higher update rate. No option changes simulation speed. See [docs/architecture.md](docs/architecture.md) for the per-rate schedules.

## Release status

| Rate | Status |
| --- | --- |
| 60 FPS | stable; long-standing baseline, live-verified |
| 120 FPS | stable; live-tested, locally accepted |
| 30 FPS | research switch of the same source tree; native engine timing, instrumented build |
| 90 FPS | research switch; live-tested, see known issue |

**Known issue**: the Jet Tengu's transformation when landing while still moving does not always play through cleanly at high rates; the snap is under investigation. Stationary landings are unaffected in current tests. What has been tested and ruled out so far is recorded in [docs/research/90fps-landing.md](docs/research/90fps-landing.md).

## Download and use

Release asset: `RA3EP1-FPS-Unlock-v1.0.0-windows-x86.zip` (32-bit Windows payloads), with `SHA256SUMS` for verification. It contains `fps60.dll`, `fps120.dll`, the loader `RA3EP1FPSUnlock.exe`, an identical loader copy named `Uprising60.exe` (for existing tooling), `LICENSE` and `SHA256SUMS`.

Run the game with a patch applied:

```text
RA3EP1FPSUnlock.exe GAME_DIRECTORY DLL_PATH
```

- `GAME_DIRECTORY` is the Uprising game root directory. The loader verifies `Data/ra3ep1_1.0.game` by SHA-256, refuses to start any other executable, and never writes the game executable.
- Payload diagnostics are written to `uprising120.log` next to the loaded DLL. Set `U60_BASELINE` to a non-empty value to keep native timing and log baseline-only measurements.

## Build from source

One source tree builds one payload per target rate; the rate is a compile-time definition and there is no runtime rate switch. Stable releases build 60 and 120; 30 and 90 remain research-only switches of the same source.

Requires CMake 3.15 or newer (Ninja recommended), Python 3, and an i686 Windows compiler for the payloads (MinGW-w64 GCC or LLVM-MinGW Clang).

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
python3 tools/release.py --build-dir build/i686 \
  --output dist/RA3EP1-FPS-Unlock-v1.0.0-windows-x86.zip
```

Full tool and option reference: [docs/usage.md](docs/usage.md).

## Testing

Verification runs in stages: per-rate host suites for the scheduling and clock math, an independent behavioral contract check ([`tests/contract_check.py`](tests/contract_check.py)), a real 32-bit bridge fixture, and live game sessions with per-window telemetry. What each stage proves — and what it deliberately does not — is described in [docs/testing.md](docs/testing.md).

## Documentation

- [docs/architecture.md](docs/architecture.md) — timing model and per-rate schedules
- [docs/usage.md](docs/usage.md) — build, release and loader reference
- [docs/testing.md](docs/testing.md) — test scope and limits
- [docs/methods.md](docs/methods.md) — first-hand analysis and verification methods
- [docs/research/90fps-landing.md](docs/research/90fps-landing.md) — open 90 FPS landing investigation
- [docs/provenance.md](docs/provenance.md) — sources, references and licensing

## Related projects

- [RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) — independent macOS compatibility project; the environment where the Windows game has been tested with these patches on Apple Silicon (Wine).

## License

Original source, tools and documentation in this repository are released under the [MIT license](LICENSE); see [NOTICE.md](NOTICE.md) for scope and third-party references. The game and its assets are proprietary and are not part of this repository. Wine-related code and licensing live in the RA3EP1Mac project; this repository contains no Wine code.
