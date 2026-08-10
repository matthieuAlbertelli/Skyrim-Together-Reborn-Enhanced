#!/usr/bin/env python3
"""Vérifie que les FormID STRE codés dans le catalogue existent dans l'ESP.

Le script ne modifie aucun fichier. Il recoupe les règles C++ de récompenses,
d'équipement, de sorts et d'aperçu avec les signatures réelles du plugin.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from audit_stre_plugin_records import PluginParseError, parse_plugin

REFERENCE_PATTERNS = (
    ("item", "ARMO", re.compile(
        r'(?:ClassItemGrantRule|OptionItemGrantRule)\{[^\n]*?'
        r'"STRE_AlternateStart\.esp",\s*(0x[0-9A-Fa-f]+)')),
    ("equipment", "ARMO", re.compile(
        r'(?:OptionEquipmentRule|EquipmentGrant)\{[^\n]*?'
        r'"STRE_AlternateStart\.esp",\s*(0x[0-9A-Fa-f]+)')),
    ("spell", "SPEL", re.compile(
        r'OptionSpellGrantRule\{[^\n]*?'
        r'"STRE_AlternateStart\.esp",\s*(0x[0-9A-Fa-f]+)')),
    ("preview", "ARMO", re.compile(
        r'PreviewFormRule\{[^\n]*?'
        r'"STRE_AlternateStart\.esp",\s*(0x[0-9A-Fa-f]+)')),
)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Recoupe le catalogue C++ et STRE_AlternateStart.esp.")
    parser.add_argument("plugin", type=Path)
    parser.add_argument("catalog", type=Path)
    parser.add_argument(
        "--client-source",
        type=Path,
        help="Source client contenant les PreviewFormRule",
    )
    args = parser.parse_args(argv)

    try:
        records, is_light = parse_plugin(args.plugin)
    except (OSError, PluginParseError) as exc:
        print(f"ERREUR: {exc}", file=sys.stderr)
        return 1

    if is_light:
        print("ERREUR: plugin ESL inattendu", file=sys.stderr)
        return 1

    by_local_id = {
        record.local_form_id: record
        for record in records
        if record.editor_id
    }

    try:
        catalog_text = args.catalog.read_text(encoding="utf-8")
        client_text = (
            args.client_source.read_text(encoding="utf-8")
            if args.client_source
            else ""
        )
    except OSError as exc:
        print(f"ERREUR: source illisible: {exc}", file=sys.stderr)
        return 1

    errors = 0
    checked: set[tuple[str, int]] = set()
    for kind, expected_signature, pattern in REFERENCE_PATTERNS:
        source = client_text if kind == "preview" else catalog_text
        for match in pattern.finditer(source):
            local_id = int(match.group(1), 0)
            key = (kind, local_id)
            if key in checked:
                continue
            checked.add(key)

            record = by_local_id.get(local_id)
            if record is None:
                errors += 1
                print(
                    f"[MANQUANT] {kind:<9} local=0x{local_id:08X} "
                    f"signature attendue={expected_signature}")
                continue

            if record.signature != expected_signature:
                errors += 1
                print(
                    f"[TYPE] {kind:<9} local=0x{local_id:08X} "
                    f"{record.editor_id}: {record.signature}, "
                    f"attendu={expected_signature}")
                continue

            print(
                f"[OK] {kind:<9} {record.signature} "
                f"local=0x{local_id:08X} {record.editor_id}")

    if not checked:
        print("ERREUR: aucune référence STRE détectée", file=sys.stderr)
        return 1

    print(f"\nRéférences contrôlées : {len(checked)}")
    if errors:
        print(f"Résultat : {errors} anomalie(s).")
        return 2

    print("Résultat : conforme.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
