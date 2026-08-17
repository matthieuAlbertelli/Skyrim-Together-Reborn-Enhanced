#!/usr/bin/env python3
"""Audit invariants for CK-managed Skyrim packaging.

This audit is intentionally independent from any maintainer-local collection
or deployment script. It validates only public repository/distribution state.
"""

from __future__ import annotations

import sys
from pathlib import Path, PurePosixPath


REPO_ROOT = Path(__file__).resolve().parents[2]
GAMEFILES_ROOT = REPO_ROOT / "GameFiles" / "Skyrim"
MANIFEST_PATH = REPO_ROOT / "GameFiles" / "STRE_AlternateStart.manifest.txt"


def normalize_relative(path: Path) -> str:
    return path.as_posix().casefold()


def main() -> int:
    errors: list[str] = []

    if not GAMEFILES_ROOT.is_dir():
        errors.append(f"Missing Skyrim GameFiles root: {GAMEFILES_ROOT}")

    if not MANIFEST_PATH.is_file():
        errors.append(f"Missing CK manifest: {MANIFEST_PATH}")

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    public_files: dict[str, str] = {}
    forbidden_pex: list[str] = []

    for file_path in GAMEFILES_ROOT.rglob("*"):
        if not file_path.is_file():
            continue

        relative = file_path.relative_to(GAMEFILES_ROOT)
        normalized = normalize_relative(relative)
        public_files[normalized] = relative.as_posix()

        parts = [part.casefold() for part in relative.parts]
        if (
            len(parts) >= 3
            and parts[0] == "scripts"
            and parts[1] == "source"
            and file_path.suffix.casefold() == ".pex"
        ):
            forbidden_pex.append(relative.as_posix())

    for relative in sorted(forbidden_pex, key=str.casefold):
        errors.append(
            "Compiled Papyrus artifact found under a source-only subtree: "
            f"GameFiles/Skyrim/{relative}"
        )

    manifest_entries: list[tuple[int, str, str]] = []
    seen_entries: dict[str, int] = {}

    with MANIFEST_PATH.open("r", encoding="utf-8-sig") as manifest:
        for line_number, raw_line in enumerate(manifest, start=1):
            value = raw_line.strip()
            if not value or value.startswith("#"):
                continue

            normalized_slashes = value.replace("\\", "/")
            pure = PurePosixPath(normalized_slashes)

            if pure.is_absolute() or ".." in pure.parts:
                errors.append(
                    f"{MANIFEST_PATH.name}:{line_number}: unsafe path: {value}"
                )
                continue

            normalized = normalized_slashes.casefold()

            if normalized in seen_entries:
                errors.append(
                    f"{MANIFEST_PATH.name}:{line_number}: duplicate entry "
                    f"(first seen at line {seen_entries[normalized]}): {value}"
                )
            else:
                seen_entries[normalized] = line_number

            if (
                normalized.startswith("scripts/source/")
                and normalized.endswith(".pex")
            ):
                errors.append(
                    f"{MANIFEST_PATH.name}:{line_number}: compiled .pex may not "
                    f"be managed from Scripts\\Source: {value}"
                )

            manifest_entries.append((line_number, value, normalized))

    for line_number, value, normalized in manifest_entries:
        if normalized not in public_files:
            errors.append(
                f"{MANIFEST_PATH.name}:{line_number}: managed file does not "
                f"exist in GameFiles/Skyrim: {value}"
            )

    if errors:
        print("CK packaging audit FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print(
        "CK packaging audit passed: "
        f"{len(manifest_entries)} managed files, "
        "0 compiled .pex files under Scripts/Source."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
