# Provenance

Where the code comes from, what was referenced, and which licenses apply.

## Original work

All source, tools, tests and documentation in this repository are original
work, released under the MIT license in [LICENSE](../LICENSE). The payload
core originated in the RA3Mac project and is credited through the retained
copyright line in LICENSE. Analysis is performed first-hand on the shipped
game executable; see [methods.md](methods.md).

## Reference material

- [SAGE-Unlocked](https://github.com/TheeHorse/SAGE-Unlocked) (GPL-3.0) —
  community research for the earlier SAGE-engine titles. Its documentation
  identified the interpolation-window and particle-update bug classes and
  helped target the investigation. Its published addresses were treated as
  unverified leads for this executable, and its code is **not** used,
  copied or adapted here; nothing in this repository derives from GPL-licensed
  source.
- OpenW3D/animation format documentation — consulted for authored animation
  format context; candidate field interpretations are labeled as candidates
  until corroborated by runtime behavior.

## Not included, and where it lives

- The game Command & Conquer: Red Alert 3 – Uprising — proprietary
  (Electronic Arts and credited rights holders). This repository contains no
  game code or data and grants no rights to it.
- Wine, Wine-derived patches, mtld3d and the macOS packaging — distributed by
  the [RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) project under its
  own notices (LGPL-2.1-or-later for Wine-derived work, zlib for mtld3d,
  OFL 1.1 for Noto Sans CJK SC). This repository contains no Wine code.

## Build toolchain

Payloads target i686 Windows and are built with [LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw)
or MinGW-w64 GCC; CI compiles the same sources with MinGW-w64. The compiler
and its runtime components keep their upstream licenses; they are not
redistributed by this project, and produced binaries link only the platform C
runtime and statically linked libgcc.

## Trademarks

Command & Conquer, Red Alert and their marks belong to their respective
owners. This project is unofficial and is not affiliated with or endorsed by
EA.
