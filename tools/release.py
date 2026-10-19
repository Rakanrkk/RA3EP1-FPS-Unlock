#!/usr/bin/env python3
"""Build the deterministic RA3EP1FPSUnlock windows-i686 release zip.

Contents: fps<rate>.dll for each configured rate, the RA3EP1FPSUnlock.exe loader
plus a compatibility alias copy (default Uprising60.exe, identical bytes),
LICENSE and README.md (when present in the repository root), and a SHA256SUMS
file. No game files, Wine components, or research diagnostics are included.

Zip entries carry fixed metadata (SOURCE_DATE_EPOCH, default
2000-01-01T00:00:00Z) and are written in sorted order, so identical inputs
produce byte-identical archives.
"""
import argparse
import hashlib
import json
import os
import time
import zipfile
from pathlib import Path


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path,
                        help="CMake build directory containing the payloads and loader")
    parser.add_argument("--output", required=True, type=Path, help="Zip file to write")
    parser.add_argument("--rates", default="60,120",
                        help="Comma-separated payload rates included in the zip")
    parser.add_argument("--loader", default="RA3EP1FPSUnlock.exe")
    parser.add_argument("--loader-alias", default="Uprising60.exe",
                        help="Second zip entry with identical loader bytes for "
                             "existing dependency documentation; empty disables")
    parser.add_argument("--source-date-epoch", type=int,
                        default=int(os.environ.get("SOURCE_DATE_EPOCH", "946684800")))
    parser.add_argument("--integration", action="store_true",
                        help="Package the canonical Mac-integration DLLs instead "
                             "of the source-built ones")
    parser.add_argument("--canonical-dir", type=Path,
                        help="Directory with fps60/uprising120.dll and "
                             "fps120/uprising120.dll canonical payloads "
                             "(requires --integration)")
    parser.add_argument("--version-name", default="v1.0.0")
    args = parser.parse_args()
    if args.integration != bool(args.canonical_dir):
        raise SystemExit("--integration requires --canonical-dir and vice versa")

    if args.integration:
        data = {}
        canonical = {}
        for rate in ("60", "120"):
            path = args.canonical_dir / f"fps{rate}" / "uprising120.dll"
            if not path.is_file():
                raise SystemExit(f"missing canonical payload: {path}")
            blob = path.read_bytes()
            canonical[f"fps{rate}.dll"] = sha256_bytes(blob)
            data[f"fps{rate}.dll"] = blob
        loader_path = args.build_dir / args.loader
        if not loader_path.is_file():
            raise SystemExit(f"missing build output: {loader_path}")
        loader_blob = loader_path.read_bytes()
        alias = args.loader_alias or "Uprising60.exe"
        data[alias] = loader_blob
        info = {
            "schema": "ra3ep1-fps-unlock-integration-v1",
            "version": args.version_name,
            "canonical_dll_sha256": canonical,
            "loader": {"name": alias,
                       "sha256": sha256_bytes(loader_blob),
                       "usage": f"{alias} GAME_DIRECTORY DLL_PATH"},
            "game_sha256": ("19b3542a7d2f7a050b16b2ea5e4ba0231b13c3fb7f"
                            "cdb2c3e89767d4d354430b"),
            "note": ("Canonical Mac-integration payload DLLs packaged with the "
                     "portable loader. Source-built payloads in the main "
                     "release zip differ from the canonical DLLs in log banner "
                     "text only; behavior contracts are verified by "
                     "tests/contract_check.py."),
        }
        data["build-info.json"] = (json.dumps(info, indent=2, sort_keys=True)
                                   + "\n").encode("utf-8")
    else:
        rates = [rate.strip() for rate in args.rates.split(",") if rate.strip()]
        members = [f"fps{rate}.dll" for rate in rates] + [args.loader]
        data = {}
        for name in members:
            path = args.build_dir / name
            if not path.is_file():
                raise SystemExit(f"missing build output: {path}")
            data[name] = path.read_bytes()

    root = Path(__file__).resolve().parents[1]
    for extra in ("LICENSE", "README.md"):
        path = root / extra
        if path.is_file():
            data[extra] = path.read_bytes()

    if args.loader_alias and not args.integration:
        if args.loader_alias in data:
            raise SystemExit(f"loader alias collides: {args.loader_alias}")
        data[args.loader_alias] = data[args.loader]

    sums = "".join(f"{sha256_bytes(blob)}  {name}\n"
                   for name, blob in sorted(data.items()))
    data["SHA256SUMS"] = sums.encode("ascii")

    stamp = time.gmtime(args.source_date_epoch)[:6]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w") as archive:
        for name in sorted(data):
            info = zipfile.ZipInfo(name, date_time=stamp)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, data[name])

    print(f"Wrote {args.output} ({args.output.stat().st_size} bytes)")
    print(sums, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
