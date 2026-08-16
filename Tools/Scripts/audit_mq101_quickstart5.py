#!/usr/bin/env python3
"""Fail-closed structural audit for STRE's MQ101 Alternate Start branches.

Reads STRE_AlternateStart.esp directly. It does not modify the plugin and does
not require the QF_MQ101 Papyrus fragment to be compiled.

This gate verifies the CK structure expected by the Vanilla Continuity spike:
- MQQuickstart remains 5;
- MQ101 stage 0 remains the validated 0/1/2/3/4/5 branch table;
- selected late MQ101 stages have vanilla == 0 plus STRE == 5 entries;
- stages 26, 28 and 145 remain untouched by MQQuickstart conditions;
- stages 250/500/800 retain three vanilla entries, each gated by == 0, plus
  exactly one STRE == 5 entry;
- stage 30 preserves the Bethesda == 1 entry;
- stage 900 preserves Complete Quest only on the vanilla == 0 entry.

This cannot prove the Papyrus body of each fragment. That is a second gate on
QF_MQ101_0003372B.psc after CK has generated the source.
"""

from __future__ import annotations

import argparse
import math
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator

RECORD_HEADER_SIZE = 24
GROUP_HEADER_SIZE = 24
COMPRESSED_FLAG = 0x00040000

MQ101_FORM_ID = 0x0003372B
MQQUICKSTART_FORM_ID = 0x0004679E
GET_GLOBAL_VALUE = 74

SIMPLE_SPLIT_STAGES = (20, 25, 40, 70, 100, 150, 180, 200)
MULTI_VANILLA_STAGES = (250, 500, 800)
UNTOUCHED_STAGES = (26, 28, 145)


class AuditError(RuntimeError):
    pass


@dataclass
class Record:
    signature: str
    form_id: int
    flags: int
    offset: int


@dataclass
class Condition:
    function: int
    param1: int
    comparison: float


@dataclass
class LogEntry:
    flags: int
    conditions: list[Condition]


@dataclass
class Stage:
    index: int
    entries: list[LogEntry]


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def record_payload(data: bytes, record: Record) -> bytes:
    size = u32(data, record.offset + 4)
    start = record.offset + RECORD_HEADER_SIZE
    payload = data[start : start + size]
    if len(payload) != size:
        raise AuditError(f"Record {record.signature} 0x{record.form_id:08X} tronqué")

    if not record.flags & COMPRESSED_FLAG:
        return payload

    if len(payload) < 4:
        raise AuditError("Record compressé invalide")

    expected = u32(payload, 0)
    try:
        expanded = zlib.decompress(payload[4:])
    except zlib.error as exc:
        raise AuditError(f"Décompression zlib impossible: {exc}") from exc

    if len(expanded) != expected:
        raise AuditError(
            f"Taille décompressée incohérente: {len(expanded)} != {expected}"
        )
    return expanded


def iter_subrecords(payload: bytes) -> Iterator[tuple[str, bytes]]:
    pos = 0
    extended_size: int | None = None

    while pos < len(payload):
        if pos + 6 > len(payload):
            raise AuditError(f"Sous-record tronqué à 0x{pos:08X}")

        sig = payload[pos : pos + 4].decode("ascii", errors="replace")
        size = u16(payload, pos + 4)
        pos += 6

        if sig == "XXXX":
            if size != 4 or pos + 4 > len(payload):
                raise AuditError("Sous-record XXXX invalide")
            extended_size = u32(payload, pos)
            pos += 4
            continue

        actual_size = extended_size if extended_size is not None else size
        extended_size = None
        end = pos + actual_size
        if end > len(payload):
            raise AuditError(f"Sous-record {sig} tronqué")

        yield sig, payload[pos:end]
        pos = end


def parse_records(data: bytes) -> list[Record]:
    if len(data) < RECORD_HEADER_SIZE or data[:4] != b"TES4":
        raise AuditError("Le fichier n'est pas un plugin TES4 valide")

    records: list[Record] = []

    def parse_range(start: int, end: int) -> None:
        pos = start
        while pos < end:
            if pos + 4 > end:
                raise AuditError("Signature de record tronquée")

            sig_bytes = data[pos : pos + 4]
            if sig_bytes == b"GRUP":
                if pos + GROUP_HEADER_SIZE > end:
                    raise AuditError("En-tête GRUP tronqué")
                size = u32(data, pos + 4)
                if size < GROUP_HEADER_SIZE or pos + size > end:
                    raise AuditError("Taille GRUP invalide")
                parse_range(pos + GROUP_HEADER_SIZE, pos + size)
                pos += size
                continue

            if pos + RECORD_HEADER_SIZE > end:
                raise AuditError("En-tête de record tronqué")

            sig = sig_bytes.decode("ascii", errors="replace")
            size = u32(data, pos + 4)
            flags = u32(data, pos + 8)
            form_id = u32(data, pos + 12)
            record_end = pos + RECORD_HEADER_SIZE + size
            if record_end > end:
                raise AuditError(f"Record {sig} dépasse son conteneur")

            records.append(Record(sig, form_id, flags, pos))
            pos = record_end

        if pos != end:
            raise AuditError("Fin de conteneur incohérente")

    parse_range(0, len(data))
    return records


def parse_global_value(data: bytes, record: Record) -> float:
    for sig, sub in iter_subrecords(record_payload(data, record)):
        if sig == "FLTV":
            if len(sub) != 4:
                raise AuditError("FLTV invalide")
            return struct.unpack("<f", sub)[0]
    raise AuditError("MQQuickstart ne contient pas FLTV")


def decode_condition(raw: bytes) -> Condition:
    if len(raw) != 32:
        raise AuditError(f"CTDA de taille inattendue: {len(raw)}")
    return Condition(
        function=u32(raw, 8),
        param1=u32(raw, 12),
        comparison=struct.unpack_from("<f", raw, 4)[0],
    )


def parse_quest_stages(data: bytes, record: Record) -> dict[int, Stage]:
    stages: dict[int, Stage] = {}
    current_stage: Stage | None = None
    current_entry: LogEntry | None = None

    for sig, sub in iter_subrecords(record_payload(data, record)):
        if sig == "INDX":
            if len(sub) < 2:
                raise AuditError("INDX invalide dans MQ101")
            index = u16(sub, 0)
            if index in stages:
                raise AuditError(f"Stage MQ101 dupliqué: {index}")
            current_stage = Stage(index=index, entries=[])
            stages[index] = current_stage
            current_entry = None
            continue

        if current_stage is None:
            continue

        if sig == "QSDT":
            if len(sub) < 1:
                raise AuditError(f"QSDT invalide au stage {current_stage.index}")
            current_entry = LogEntry(flags=sub[0], conditions=[])
            current_stage.entries.append(current_entry)
            continue

        if sig == "CTDA" and current_entry is not None:
            current_entry.conditions.append(decode_condition(sub))

    return stages


def quickstart_values(entry: LogEntry) -> list[float]:
    return [
        c.comparison
        for c in entry.conditions
        if c.function == GET_GLOBAL_VALUE and c.param1 == MQQUICKSTART_FORM_ID
    ]


def has_qs(entry: LogEntry, value: float) -> bool:
    return any(math.isclose(v, value, abs_tol=0.0001) for v in quickstart_values(entry))


def fmt_entry(entry: LogEntry) -> str:
    qs = quickstart_values(entry)
    qs_text = ",".join(f"{v:g}" for v in qs) if qs else "-"
    return f"flags=0x{entry.flags:02X} MQQuickstart=[{qs_text}] conds={len(entry.conditions)}"


class Reporter:
    def __init__(self) -> None:
        self.errors = 0

    def ok(self, text: str) -> None:
        print(f"[OK]   {text}")

    def fail(self, text: str) -> None:
        self.errors += 1
        print(f"[FAIL] {text}")

    def require(self, condition: bool, ok: str, fail: str) -> None:
        if condition:
            self.ok(ok)
        else:
            self.fail(fail)


def audit_stage_exact_qs(
    rep: Reporter,
    stages: dict[int, Stage],
    stage_index: int,
    expected_entry_count: int,
    expected_values: list[float],
) -> None:
    stage = stages.get(stage_index)
    if stage is None:
        rep.fail(f"MQ101 stage {stage_index}: absent")
        return

    rep.require(
        len(stage.entries) == expected_entry_count,
        f"stage {stage_index}: {len(stage.entries)} Log Entries",
        f"stage {stage_index}: {len(stage.entries)} Log Entries, attendu {expected_entry_count}",
    )

    actual_values: list[float] = []
    bad_multi = False
    for entry in stage.entries:
        vals = quickstart_values(entry)
        if len(vals) > 1:
            bad_multi = True
        actual_values.extend(vals)

    actual_values.sort()
    expected_sorted = sorted(expected_values)

    rep.require(
        not bad_multi,
        f"stage {stage_index}: au plus une condition MQQuickstart par Log Entry",
        f"stage {stage_index}: une Log Entry contient plusieurs conditions MQQuickstart",
    )
    rep.require(
        len(actual_values) == len(expected_sorted)
        and all(math.isclose(a, b, abs_tol=0.0001) for a, b in zip(actual_values, expected_sorted)),
        f"stage {stage_index}: branches MQQuickstart {actual_values}",
        f"stage {stage_index}: branches MQQuickstart {actual_values}, attendu {expected_sorted}",
    )

    for i, entry in enumerate(stage.entries):
        print(f"       entry {i}: {fmt_entry(entry)}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Audit fail-closed des branches MQ101/MQQuickstart du spike STRE."
    )
    parser.add_argument("plugin", type=Path, help="Chemin vers STRE_AlternateStart.esp")
    args = parser.parse_args()

    if not args.plugin.is_file():
        print(f"[FAIL] Plugin introuvable: {args.plugin}", file=sys.stderr)
        return 2

    try:
        data = args.plugin.read_bytes()
        records = parse_records(data)
        by_form_id = {r.form_id: r for r in records}

        mq101 = by_form_id.get(MQ101_FORM_ID)
        quickstart = by_form_id.get(MQQUICKSTART_FORM_ID)
        if mq101 is None or mq101.signature != "QUST":
            raise AuditError("Override MQ101 [0003372B] absent")
        if quickstart is None or quickstart.signature != "GLOB":
            raise AuditError("Override MQQuickstart [0004679E] absent")

        rep = Reporter()

        qsv = parse_global_value(data, quickstart)
        rep.require(
            math.isclose(qsv, 5.0, abs_tol=0.0001),
            "MQQuickstart = 5",
            f"MQQuickstart = {qsv:g}, attendu 5",
        )

        stages = parse_quest_stages(data, mq101)

        # Validated #65 bootstrap must remain unchanged.
        audit_stage_exact_qs(rep, stages, 0, 6, [0, 1, 2, 3, 4, 5])

        # One vanilla entry gated ==0 + one STRE entry gated ==5.
        for s in SIMPLE_SPLIT_STAGES:
            audit_stage_exact_qs(rep, stages, s, 2, [0, 5])

        # Stage 30 keeps Bethesda's ==1 branch and gains ==0 / ==5.
        audit_stage_exact_qs(rep, stages, 30, 3, [0, 1, 5])

        # These must stay structurally untouched by the current increment.
        for s in UNTOUCHED_STAGES:
            stage = stages.get(s)
            if stage is None:
                rep.fail(f"MQ101 stage {s}: absent")
                continue
            qs = [v for e in stage.entries for v in quickstart_values(e)]
            rep.require(
                len(stage.entries) == 1 and not qs,
                f"stage {s}: intact (1 Log Entry, aucune condition MQQuickstart)",
                f"stage {s}: modifié de façon inattendue ({len(stage.entries)} entries, MQQuickstart={qs})",
            )
            for i, entry in enumerate(stage.entries):
                print(f"       entry {i}: {fmt_entry(entry)}")

        # Three existing vanilla entries, each gated ==0, plus one STRE ==5.
        for s in MULTI_VANILLA_STAGES:
            audit_stage_exact_qs(rep, stages, s, 4, [0, 0, 0, 5])

            stage = stages.get(s)
            if stage:
                zero_entries = sum(has_qs(e, 0) for e in stage.entries)
                five_entries = sum(has_qs(e, 5) for e in stage.entries)
                rep.require(
                    zero_entries == 3 and five_entries == 1,
                    f"stage {s}: 3 vanilla ==0 + 1 STRE ==5",
                    f"stage {s}: attendu 3 entries ==0 et 1 entry ==5, obtenu {zero_entries}/{five_entries}",
                )

        # Stage 900: vanilla Complete Quest stays on ==0; STRE ==5 must not carry it.
        audit_stage_exact_qs(rep, stages, 900, 2, [0, 5])
        s900 = stages.get(900)
        if s900:
            e0 = [e for e in s900.entries if has_qs(e, 0)]
            e5 = [e for e in s900.entries if has_qs(e, 5)]
            rep.require(
                len(e0) == 1 and bool(e0[0].flags & 0x01),
                "stage 900: Complete Quest conservé sur la branche vanilla ==0",
                "stage 900: la branche vanilla ==0 doit conserver Complete Quest",
            )
            rep.require(
                len(e5) == 1 and not bool(e5[0].flags & 0x01),
                "stage 900: branche STRE ==5 sans Complete Quest",
                "stage 900: la branche STRE ==5 ne doit PAS avoir Complete Quest",
            )

        print()
        if rep.errors:
            print(f"RÉSULTAT: ÉCHEC — {rep.errors} anomalie(s). Ne compile pas.")
            return 1

        print("RÉSULTAT: CONFORME — structure MQ101 prête pour le gate Papyrus.")
        print("Ce résultat ne valide pas encore le corps des fragments QF_MQ101.")
        return 0

    except (OSError, AuditError, struct.error) as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
