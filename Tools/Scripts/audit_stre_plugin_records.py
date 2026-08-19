#!/usr/bin/env python3
"""Audit des records STRE et des overrides de master approuvés.

Le script lit directement les en-têtes TES4/GRUP/record, décompresse les
records marqués COMPRESSED et extrait les sous-records EDID. Il ne modifie
jamais le plugin.
"""

from __future__ import annotations

import argparse
import csv
import json
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator

RECORD_HEADER_SIZE = 24
GROUP_HEADER_SIZE = 24
COMPRESSED_FLAG = 0x00040000
ESL_FLAG = 0x00000200


class PluginParseError(RuntimeError):
    pass


@dataclass(frozen=True)
class PluginRecord:
    signature: str
    form_id: int
    flags: int
    editor_id: str | None
    full_name: str | None
    offset: int

    @property
    def local_form_id(self) -> int:
        return self.form_id & 0x00FFFFFF


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _decode_zstring(raw: bytes) -> str:
    raw = raw.split(b"\x00", 1)[0]
    for encoding in ("utf-8", "cp1252"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return raw.decode("latin-1", errors="replace")


def _record_payload(data: bytes, data_offset: int, data_size: int, flags: int) -> bytes:
    payload = data[data_offset : data_offset + data_size]
    if len(payload) != data_size:
        raise PluginParseError(
            f"Record tronqué à 0x{data_offset - RECORD_HEADER_SIZE:08X}: "
            f"{len(payload)} octets lus, {data_size} attendus"
        )

    if not flags & COMPRESSED_FLAG:
        return payload

    if len(payload) < 4:
        raise PluginParseError("Record compressé sans taille décompressée")

    expected_size = _u32(payload, 0)
    try:
        expanded = zlib.decompress(payload[4:])
    except zlib.error as exc:
        raise PluginParseError(f"Échec de décompression zlib: {exc}") from exc

    if len(expanded) != expected_size:
        raise PluginParseError(
            f"Taille décompressée incohérente: {len(expanded)} obtenus, "
            f"{expected_size} attendus"
        )
    return expanded


def _iter_subrecords(payload: bytes) -> Iterator[tuple[str, bytes]]:
    position = 0
    extended_size: int | None = None

    while position < len(payload):
        if position + 6 > len(payload):
            raise PluginParseError(
                f"Sous-record tronqué à l'offset relatif 0x{position:08X}"
            )

        signature = payload[position : position + 4].decode("ascii", errors="replace")
        size = _u16(payload, position + 4)
        position += 6

        if signature == "XXXX":
            if size != 4 or position + 4 > len(payload):
                raise PluginParseError("Sous-record XXXX invalide")
            extended_size = _u32(payload, position)
            position += 4
            continue

        effective_size = extended_size if extended_size is not None else size
        extended_size = None
        end = position + effective_size
        if end > len(payload):
            raise PluginParseError(
                f"Sous-record {signature} tronqué: fin 0x{end:08X}, "
                f"payload 0x{len(payload):08X}"
            )

        yield signature, payload[position:end]
        position = end


def _extract_text_subrecord(payload: bytes, expected: str) -> str | None:
    for signature, subdata in _iter_subrecords(payload):
        if signature != expected:
            continue
        # A localized plugin stores a 32-bit string-table identifier instead
        # of inline text. STRE_AlternateStart.esp is currently non-localized.
        if len(subdata) == 4 and expected == "FULL":
            return None
        return _decode_zstring(subdata)
    return None


def _extract_editor_id(payload: bytes) -> str | None:
    return _extract_text_subrecord(payload, "EDID")


def _extract_full_name(payload: bytes) -> str | None:
    return _extract_text_subrecord(payload, "FULL")


def _extract_global_value(data: bytes, record: PluginRecord) -> float | None:
    if record.signature != "GLOB":
        return None

    data_size = _u32(data, record.offset + 4)
    payload = _record_payload(
        data,
        record.offset + RECORD_HEADER_SIZE,
        data_size,
        record.flags,
    )
    for signature, subdata in _iter_subrecords(payload):
        if signature != "FLTV":
            continue
        if len(subdata) != 4:
            raise PluginParseError(
                f"FLTV invalide dans {record.editor_id}: "
                f"{len(subdata)} octets")
        return struct.unpack("<f", subdata)[0]
    return None


def parse_plugin(path: Path) -> tuple[list[PluginRecord], bool]:
    data = path.read_bytes()
    if len(data) < RECORD_HEADER_SIZE or data[:4] != b"TES4":
        raise PluginParseError(f"{path} n'est pas un plugin TES4 valide")

    records: list[PluginRecord] = []
    tes4_flags = _u32(data, 8)
    is_light = bool(tes4_flags & ESL_FLAG)

    def parse_range(start: int, end: int) -> None:
        position = start
        while position < end:
            if position + 4 > end:
                raise PluginParseError(f"Signature tronquée à 0x{position:08X}")

            signature_bytes = data[position : position + 4]
            if signature_bytes == b"GRUP":
                if position + GROUP_HEADER_SIZE > end:
                    raise PluginParseError(f"En-tête GRUP tronqué à 0x{position:08X}")
                group_size = _u32(data, position + 4)
                if group_size < GROUP_HEADER_SIZE:
                    raise PluginParseError(
                        f"Taille GRUP invalide ({group_size}) à 0x{position:08X}"
                    )
                group_end = position + group_size
                if group_end > end:
                    raise PluginParseError(
                        f"GRUP dépasse son conteneur à 0x{position:08X}"
                    )
                parse_range(position + GROUP_HEADER_SIZE, group_end)
                position = group_end
                continue

            if position + RECORD_HEADER_SIZE > end:
                raise PluginParseError(f"En-tête record tronqué à 0x{position:08X}")

            signature = signature_bytes.decode("ascii", errors="replace")
            data_size = _u32(data, position + 4)
            flags = _u32(data, position + 8)
            form_id = _u32(data, position + 12)
            data_offset = position + RECORD_HEADER_SIZE
            record_end = data_offset + data_size
            if record_end > end:
                raise PluginParseError(
                    f"Record {signature} dépasse son conteneur à 0x{position:08X}"
                )

            payload = _record_payload(data, data_offset, data_size, flags)
            editor_id = _extract_editor_id(payload)
            full_name = _extract_full_name(payload)
            records.append(
                PluginRecord(
                    signature,
                    form_id,
                    flags,
                    editor_id,
                    full_name,
                    position,
                )
            )
            position = record_end

        if position != end:
            raise PluginParseError(
                f"Fin de conteneur incohérente: 0x{position:08X} != 0x{end:08X}"
            )

    parse_range(0, len(data))
    return records, is_light


def _extract_effect_items(
    data: bytes,
    record: PluginRecord,
    by_form_id: dict[int, PluginRecord],
) -> list[dict]:
    data_size = _u32(data, record.offset + 4)
    payload = _record_payload(
        data,
        record.offset + RECORD_HEADER_SIZE,
        data_size,
        record.flags,
    )

    effects: list[dict] = []
    current: dict | None = None
    for signature, subdata in _iter_subrecords(payload):
        if signature == "EFID":
            if current is not None:
                effects.append(current)
            effect_form_id = _u32(subdata, 0)
            effect_record = by_form_id.get(effect_form_id)
            current = {
                "effectFormId": effect_form_id,
                "effectEditorId": (
                    effect_record.editor_id if effect_record else None),
                "conditionCount": 0,
            }
        elif signature == "EFIT" and current is not None:
            if len(subdata) != 12:
                raise PluginParseError(
                    f"EFIT invalide dans {record.editor_id}: {len(subdata)} octets")
            magnitude, area, duration = struct.unpack("<fII", subdata)
            current.update(
                magnitude=magnitude,
                area=area,
                duration=duration,
            )
        elif signature == "CTDA" and current is not None:
            current["conditionCount"] += 1

    if current is not None:
        effects.append(current)
    return effects


def load_manifest(path: Path) -> dict:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise PluginParseError(f"Manifest illisible: {exc}") from exc

    if not isinstance(manifest, dict) or not isinstance(manifest.get("records"), list):
        raise PluginParseError("Le manifest doit contenir un tableau 'records'")

    for field in (
        "approvedMasterOverrides",
        "allowedMasterRecordsWithoutEditorId",
    ):
        value = manifest.get(field, [])
        if not isinstance(value, list):
            raise PluginParseError(
                f"Le champ {field!r} doit être un tableau")
    return manifest


def _master_name_for_record(
    record: PluginRecord,
    masters: list[str],
) -> str | None:
    master_index = record.form_id >> 24
    if master_index >= len(masters):
        return None
    return masters[master_index]


def _expected_master_record_key(entry: dict) -> tuple[str, int, str | None]:
    signature = entry.get("signature")
    expected_form_id = entry.get("expectedFormId")
    if not isinstance(signature, str) or not signature:
        raise PluginParseError(
            "Un override de master doit définir 'signature'")
    if expected_form_id is None:
        raise PluginParseError(
            f"L'override {signature} doit définir 'expectedFormId'")

    try:
        form_id = int(str(expected_form_id), 0)
    except ValueError as exc:
        raise PluginParseError(
            f"FormID de master invalide pour {signature}: "
            f"{expected_form_id!r}") from exc

    editor_id = entry.get("editorId")
    if editor_id is not None and (
        not isinstance(editor_id, str) or not editor_id
    ):
        raise PluginParseError(
            f"EditorID invalide pour l'override {signature}")

    return signature, form_id, editor_id


def build_cpp_lines(entry: dict, record: PluginRecord, plugin_name: str) -> list[str]:
    grant = entry.get("grant")
    if not isinstance(grant, dict):
        return []

    count = int(grant.get("count", 1))
    form_id = f"0x{record.local_form_id:08X}"
    kind = grant.get("kind")
    identifier = grant.get("id")
    if not isinstance(identifier, str) or not identifier:
        return []

    lines: list[str] = []
    if kind == "option":
        lines.append(
            f'OptionItemGrantRule{{"{identifier}", "{plugin_name}", {form_id}, {count}}},'
        )
        equip = entry.get("equip")
        if isinstance(equip, dict):
            side = equip.get("side")
            if side in {"Right", "Left"}:
                lines.append(
                    f'OptionEquipmentRule{{"{identifier}", "{plugin_name}", '
                    f'{form_id}, {count}, EquipmentSide::{side}}},'
                )
    elif kind == "class":
        cpp_class_id = grant.get("cppClassId")
        if isinstance(cpp_class_id, str) and cpp_class_id:
            lines.append(
                f'ClassItemGrantRule{{{cpp_class_id}, "{plugin_name}", {form_id}, {count}}},'
            )
    return lines


def write_tsv(path: Path, rows: Iterable[dict]) -> None:
    fieldnames = [
        "status",
        "signature",
        "editor_id",
        "local_form_id",
        "loaded_form_id",
        "offset",
        "expected_signature",
        "display_name",
        "actual_full_name",
    ]
    with path.open("w", encoding="utf-8-sig", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames, delimiter="\t")
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Audite les EditorID STRE présents dans un plugin TES4."
    )
    parser.add_argument("plugin", type=Path, help="Chemin du plugin ESP/ESM")
    parser.add_argument("--manifest", type=Path, help="Manifest JSON des records attendus")
    parser.add_argument("--prefix", default="STRE_", help="Préfixe EditorID à lister")
    parser.add_argument("--output", type=Path, help="Rapport TSV à écrire")
    parser.add_argument(
        "--reject-unexpected",
        action="store_true",
        help="Signale comme anomalie tout EditorID préfixé absent du manifest",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Retourne un code non nul si un record attendu manque ou a le mauvais type",
    )
    args = parser.parse_args(argv)

    try:
        records, is_light = parse_plugin(args.plugin)
    except (OSError, PluginParseError) as exc:
        print(f"ERREUR: {exc}", file=sys.stderr)
        return 1

    if is_light:
        print(
            "ERREUR: le plugin porte le flag ESL. Le catalogue actuel attend des "
            "FormID locaux de plugin non-light; ne pas intégrer avant arbitrage.",
            file=sys.stderr,
        )
        return 1

    by_editor_id = {
        record.editor_id: record
        for record in records
        if record.editor_id is not None
    }
    by_form_id = {record.form_id: record for record in records}

    data = args.plugin.read_bytes()
    tes4_size = _u32(data, 4)
    tes4_flags = _u32(data, 8)
    tes4_payload = _record_payload(
        data, RECORD_HEADER_SIZE, tes4_size, tes4_flags)
    masters = [
        _decode_zstring(subdata)
        for signature, subdata in _iter_subrecords(tes4_payload)
        if signature == "MAST"
    ]

    manifest: dict | None = None
    expected: list[dict] = []
    approved_master_overrides: list[dict] = []
    allowed_master_records_without_editor_id: list[dict] = []
    if args.manifest:
        try:
            manifest = load_manifest(args.manifest)
        except PluginParseError as exc:
            print(f"ERREUR: {exc}", file=sys.stderr)
            return 1
        expected = manifest["records"]
        approved_master_overrides = manifest.get(
            "approvedMasterOverrides", [])
        allowed_master_records_without_editor_id = manifest.get(
            "allowedMasterRecordsWithoutEditorId", [])

    report_rows: list[dict] = []
    errors = 0

    if manifest is not None:
        required_masters = manifest.get("requiredMasters", [])
        if required_masters and masters != required_masters:
            errors += 1
            print(
                "[MASTERS] obtenus=" + repr(masters) +
                " attendus=" + repr(required_masters)
            )

    if expected:
        print(f"Plugin : {args.plugin}")
        print(f"Records STRE attendus : {len(expected)}")
        for entry in expected:
            editor_id = entry.get("editorId")
            expected_signature = entry.get("signature")
            display_name = entry.get("displayName", "")
            expected_local_form_id = entry.get("expectedLocalFormId")
            record = by_editor_id.get(editor_id)
            if record is None:
                status = "MISSING"
                errors += 1
                print(f"[MANQUANT] {expected_signature:<4} {editor_id}")
                report_rows.append(
                    {
                        "status": status,
                        "signature": "",
                        "editor_id": editor_id,
                        "local_form_id": "",
                        "loaded_form_id": "",
                        "offset": "",
                        "expected_signature": expected_signature,
                        "display_name": display_name,
                        "actual_full_name": "",
                    }
                )
                continue

            problems: list[str] = []
            if record.signature != expected_signature:
                problems.append(
                    f"type={record.signature}, attendu={expected_signature}")

            if expected_local_form_id is not None:
                expected_local = int(str(expected_local_form_id), 0)
                if record.local_form_id != expected_local:
                    problems.append(
                        f"local=0x{record.local_form_id:08X}, "
                        f"attendu=0x{expected_local:08X}")

            if record.full_name is not None and record.full_name != record.full_name.strip():
                problems.append("FULL contient des espaces ou retours de ligne périphériques")

            if display_name and record.full_name != display_name:
                problems.append(
                    f"FULL={record.full_name!r}, attendu={display_name!r}")

            if problems:
                status = "INVALID"
                errors += 1
                print(f"[INVALIDE] {editor_id}: " + "; ".join(problems))
            else:
                status = "OK"
                print(
                    f"[OK] {record.signature:<4} {editor_id:<58} "
                    f"local=0x{record.local_form_id:08X}"
                )

            report_rows.append(
                {
                    "status": status,
                    "signature": record.signature,
                    "editor_id": editor_id,
                    "local_form_id": f"0x{record.local_form_id:08X}",
                    "loaded_form_id": f"0x{record.form_id:08X}",
                    "offset": f"0x{record.offset:08X}",
                    "expected_signature": expected_signature,
                    "display_name": display_name,
                    "actual_full_name": record.full_name or "",
                }
            )

        if args.reject_unexpected:
            expected_ids = {entry.get("editorId") for entry in expected}
            unexpected = sorted(
                (
                    record
                    for record in records
                    if record.editor_id
                       and record.editor_id.startswith(args.prefix)
                       and record.editor_id not in expected_ids
                ),
                key=lambda record: (
                    record.signature,
                    record.editor_id or "",
                    record.local_form_id,
                ),
            )
            for record in unexpected:
                errors += 1
                print(
                    f"[INATTENDU] {record.signature:<4} "
                    f"{record.editor_id} local=0x{record.local_form_id:08X}"
                )
                report_rows.append(
                    {
                        "status": "UNEXPECTED",
                        "signature": record.signature,
                        "editor_id": record.editor_id,
                        "local_form_id": f"0x{record.local_form_id:08X}",
                        "loaded_form_id": f"0x{record.form_id:08X}",
                        "offset": f"0x{record.offset:08X}",
                        "expected_signature": "",
                        "display_name": "",
                        "actual_full_name": record.full_name or "",
                    }
                )

        approved_master_entries = (
            approved_master_overrides +
            allowed_master_records_without_editor_id
        )
        approved_master_keys: set[tuple[str, int, str | None]] = set()

        for entry in approved_master_entries:
            try:
                expected_signature, expected_form_id, expected_editor_id = (
                    _expected_master_record_key(entry)
                )
            except PluginParseError as exc:
                errors += 1
                print(f"[MANIFEST] {exc}")
                continue

            key = (
                expected_signature,
                expected_form_id,
                expected_editor_id,
            )
            if key in approved_master_keys:
                errors += 1
                print(
                    "[MANIFEST] Override de master dupliqué: "
                    f"{expected_signature} "
                    f"0x{expected_form_id:08X} "
                    f"{expected_editor_id or '<sans EDID>'}"
                )
                continue
            approved_master_keys.add(key)

            record = by_form_id.get(expected_form_id)
            if record is None:
                errors += 1
                print(
                    f"[OVERRIDE MANQUANT] {expected_signature:<4} "
                    f"0x{expected_form_id:08X} "
                    f"{expected_editor_id or '<sans EDID>'}"
                )
                continue

            problems: list[str] = []
            if record.signature != expected_signature:
                problems.append(
                    f"type={record.signature}, "
                    f"attendu={expected_signature}"
                )
            if record.editor_id != expected_editor_id:
                problems.append(
                    f"EDID={record.editor_id!r}, "
                    f"attendu={expected_editor_id!r}"
                )

            expected_master = entry.get("master")
            actual_master = _master_name_for_record(
                record, masters)
            if expected_master and actual_master != expected_master:
                problems.append(
                    f"master={actual_master!r}, "
                    f"attendu={expected_master!r}"
                )

            if "expectedValue" in entry:
                if record.signature != "GLOB":
                    problems.append(
                        "expectedValue n'est valide que pour un GLOB")
                else:
                    try:
                        actual_value = _extract_global_value(
                            data, record)
                    except PluginParseError as exc:
                        problems.append(str(exc))
                    else:
                        expected_value = float(entry["expectedValue"])
                        if actual_value is None:
                            problems.append("FLTV absent")
                        elif abs(actual_value - expected_value) > 0.0001:
                            problems.append(
                                f"value={actual_value!r}, "
                                f"attendu={expected_value!r}"
                            )

            if problems:
                errors += 1
                print(
                    f"[OVERRIDE INVALIDE] "
                    f"0x{expected_form_id:08X}: " +
                    "; ".join(problems)
                )
                continue

            print(
                f"[OK OVERRIDE] {record.signature:<4} "
                f"{record.editor_id or '<sans EDID>':<58} "
                f"form=0x{record.form_id:08X} "
                f"master={actual_master or '<self>'}"
            )
            report_rows.append(
                {
                    "status": "OVERRIDE_OK",
                    "signature": record.signature,
                    "editor_id": record.editor_id or "",
                    "local_form_id": (
                        f"0x{record.local_form_id:08X}"),
                    "loaded_form_id": (
                        f"0x{record.form_id:08X}"),
                    "offset": f"0x{record.offset:08X}",
                    "expected_signature": expected_signature,
                    "display_name": "",
                    "actual_full_name": record.full_name or "",
                }
            )

        if args.reject_unexpected and manifest is not None:
            master_records = [
                record
                for record in records
                if record.signature != "TES4"
                and _master_name_for_record(record, masters) is not None
            ]
            unexpected_master_records = sorted(
                (
                    record
                    for record in master_records
                    if (
                        record.signature,
                        record.form_id,
                        record.editor_id,
                    ) not in approved_master_keys
                ),
                key=lambda record: (
                    record.form_id,
                    record.signature,
                    record.editor_id or "",
                ),
            )
            for record in unexpected_master_records:
                errors += 1
                print(
                    f"[OVERRIDE INATTENDU] {record.signature:<4} "
                    f"0x{record.form_id:08X} "
                    f"{record.editor_id or '<sans EDID>'} "
                    f"master={_master_name_for_record(record, masters)}"
                )
                report_rows.append(
                    {
                        "status": "UNEXPECTED_OVERRIDE",
                        "signature": record.signature,
                        "editor_id": record.editor_id or "",
                        "local_form_id": (
                            f"0x{record.local_form_id:08X}"),
                        "loaded_form_id": (
                            f"0x{record.form_id:08X}"),
                        "offset": f"0x{record.offset:08X}",
                        "expected_signature": "",
                        "display_name": "",
                        "actual_full_name": record.full_name or "",
                    }
                )

        for spell_editor_id, effect_expectations in (
            manifest.get("spellEffectExpectations", {}).items()
        ):
            spell_record = by_editor_id.get(spell_editor_id)
            if spell_record is None or spell_record.signature != "SPEL":
                continue

            actual_effects = _extract_effect_items(
                data, spell_record, by_form_id)
            if len(actual_effects) != len(effect_expectations):
                errors += 1
                print(
                    f"[EFFETS] {spell_editor_id}: "
                    f"{len(actual_effects)} effet(s), "
                    f"{len(effect_expectations)} attendu(s)")
                continue

            for index, (actual, expected_effect) in enumerate(
                zip(actual_effects, effect_expectations)
            ):
                problems: list[str] = []
                for field in (
                    "effectEditorId",
                    "area",
                    "duration",
                    "conditionCount",
                ):
                    if field in expected_effect and (
                        actual.get(field) != expected_effect[field]
                    ):
                        problems.append(
                            f"{field}={actual.get(field)!r}, "
                            f"attendu={expected_effect[field]!r}")

                if "magnitude" in expected_effect and abs(
                    float(actual.get("magnitude", 0.0)) -
                    float(expected_effect["magnitude"])
                ) > 0.001:
                    problems.append(
                        f"magnitude={actual.get('magnitude')!r}, "
                        f"attendu={expected_effect['magnitude']!r}")

                if problems:
                    errors += 1
                    print(
                        f"[EFFET INVALIDE] {spell_editor_id}[{index}]: " +
                        "; ".join(problems))
                else:
                    print(
                        f"[OK EFFET] {spell_editor_id}[{index}] "
                        f"{actual.get('effectEditorId') or hex(actual['effectFormId'])}")

        present_entries = [
            (entry, by_editor_id.get(entry.get("editorId")))
            for entry in expected
        ]
        cpp_lines = [
            line
            for entry, record in present_entries
            if record is not None and record.signature == entry.get("signature")
            for line in build_cpp_lines(
                entry,
                record,
                manifest.get("pluginName", args.plugin.name) if manifest else args.plugin.name,
            )
        ]
        if cpp_lines:
            print("\nLignes C++ générées :")
            for line in cpp_lines:
                print(line)
    else:
        selected = sorted(
            (
                record
                for record in records
                if record.editor_id and record.editor_id.startswith(args.prefix)
            ),
            key=lambda record: (record.signature, record.editor_id or ""),
        )
        print(f"Plugin : {args.plugin}")
        print(f"Records avec préfixe {args.prefix!r} : {len(selected)}")
        for record in selected:
            print(
                f"{record.signature:<4} 0x{record.local_form_id:08X} "
                f"{record.editor_id}"
            )
            report_rows.append(
                {
                    "status": "FOUND",
                    "signature": record.signature,
                    "editor_id": record.editor_id,
                    "local_form_id": f"0x{record.local_form_id:08X}",
                    "loaded_form_id": f"0x{record.form_id:08X}",
                    "offset": f"0x{record.offset:08X}",
                    "expected_signature": "",
                    "display_name": "",
                    "actual_full_name": record.full_name or "",
                }
            )

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_tsv(args.output, report_rows)
        print(f"\nRapport TSV : {args.output}")

    if errors:
        print(f"\nRésultat : {errors} anomalie(s).")
        return 2 if args.strict else 0

    print("\nRésultat : conforme.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
