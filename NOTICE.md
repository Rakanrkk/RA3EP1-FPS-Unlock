# Notices

This file describes what the MIT license in [LICENSE](LICENSE) covers and what
it does not.

## Covered

Original source and tools in this repository, released under the MIT license:

- Payload source: `src/payload.c` and the headers it builds from
  (`src/schedule120.h`, `src/particle_clock.h`, `src/sway_compat.h`,
  `src/rate_config.h`, `src/model_trace.h`).
- Loader: `src/loader.c`, shipped as `RA3EP1FPSUnlock.exe`; release archives
  include an identical copy named `Uprising60.exe` for existing tooling.
- Build and packaging files: `CMakeLists.txt`, `cmake/i686-windows.cmake`,
  and `tools/release.py`.
- Test suites under `tests/`, and the documentation under `docs/`.

The payload core originated in the RA3Mac project; its authors are credited
through the retained `RA3Mac contributors` copyright line in LICENSE. New
original contributions are accepted under the same MIT license.

## Not covered

- The game Command & Conquer: Red Alert 3 – Uprising and its executable, data
  and artwork remain the property of Electronic Arts and the credited rights
  holders. This repository contains none of them — only interface constants
  (addresses, expected instruction bytes, and the game executable SHA-256)
  used as edit guards — and grants no rights to them.
- Wine and Wine-derived patches are not part of this repository. They are
  distributed and licensed (LGPL-2.1-or-later) through the
  [RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) project, whose
  THIRD-PARTY-NOTICES.md and LICENSES/ directory govern those components.
- mtld3d, Noto Sans CJK SC, LLVM-MinGW, MinGW-w64 and other build or runtime
  dependencies keep their upstream licenses; they are not redistributed here.

## References

Pattern-location references consulted during analysis are listed in
[docs/provenance.md](docs/provenance.md). Referencing an external project's
published findings does not copy its code or import its license obligations
into this repository; GPL-licensed reference code was not used here.
