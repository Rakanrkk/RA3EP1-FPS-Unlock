# Contributing

Reproducible reports and focused patches are welcome.

## Reporting issues

- Use the issue templates. For gameplay observations, always state the frame
  rate (30/60/90/120), the in-game scenario, and what you expected versus what
  happened.
- Attach only privacy-cleaned logs. Never attach game files, saves, replays of
  unreleased content, or anything containing personal data or local paths.
- One scenario per report is easier to reproduce than a combined one.

## Submitting changes

- Keep patches focused: one subsystem per change, with the affected native
  paths named.
- Every timing edit must carry an exact byte-level identity guard so it only
  applies where the native bytes match. Unconditional edits are not accepted.
- Run the host suites and `tests/contract_check.py` for the affected rates
  before submitting; add or extend a check when you add or change behavior.
- Keep the evidence classes separate in claims: compilation, host tests,
  contract checks, 32-bit bridge fixtures and live game sessions are
  different levels of evidence. Say which one backs each claim.
- Diagnostic switches (`RA3_TARGET_FPS=30|90`, `RA3_DIAGNOSTIC_NATIVE_SWAY=1`,
  `RA3_DIAGNOSTIC_UNIT_WINDOW=10`, `RA3_MODEL_TRACE=1`) are investigation
  tools; do not present them as release profiles.

## Scope and licensing

- New original source is submitted under this repository's MIT license.
- Do not include GPL-licensed code, including from reference projects; pattern
  references belong in documentation with attribution, not in source.
- Do not commit game files, compiled payloads, toolchains or session logs.
