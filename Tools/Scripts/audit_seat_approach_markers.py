#!/usr/bin/env python3
"""Read-only audit of existing MarkerXX / SeatXX approach bindings.

Never creates/moves plugin records or claims collision/visual validation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

from audit_mq101_quickstart5 import parse_records, record_payload, iter_subrecords, AuditError

ROOT = Path(__file__).resolve().parents[2]
PLUGIN = ROOT / "GameFiles/Skyrim/STRE_AlternateStart.esp"
PREFIX = "STRE_REFR_PlayerCreationMarker"


def audit(data: bytes) -> tuple[list[dict], list[str]]:
    records = parse_records(data)
    cells = {}

    # parse_records has already validated container lengths. Keep owning CELL
    # from cell children/subgroups; EditorIDs alone cannot prove the right cell.
    def parents(start, end, cell=None):
        pos = start
        while pos < end:
            size = struct.unpack_from('<I', data, pos + 4)[0]
            if data[pos:pos + 4] == b'GRUP':
                label, kind = struct.unpack_from('<II', data, pos + 8)
                parents(pos + 24, pos + size, label if kind in (6, 8, 9, 10) else cell)
                pos += size
            else:
                cells[pos] = cell
                pos += 24 + size
    parents(0, len(data))
    fields = {r.offset: dict(iter_subrecords(record_payload(data, r))) for r in records}
    def edid(r): return fields[r.offset].get('EDID', b'').rstrip(b'\0').decode('utf-8')
    by_name = {}
    for r in records:
        by_name.setdefault(edid(r), []).append(r)
    masters = [value.rstrip(b'\0').decode('utf-8') for name, value in
               iter_subrecords(record_payload(data, records[0])) if name == 'MAST']
    errors, result = [], []
    if records[0].flags & 0x200:
        errors.append('ESL conversion is outside this binding contract')
    if 'Skyrim.esm' not in masters:
        return [], errors + ['Skyrim.esm master missing']
    base = (masters.index('Skyrim.esm') << 24) | 0x34
    cell = by_name.get('STRE_CELL_AlternateStart', [])
    if len(cell) != 1 or cell[0].signature != 'CELL':
        return [], errors + ['STRE_CELL_AlternateStart missing/ambiguous']
    seen_ids, seen_transforms = set(), set()
    names = {f'{PREFIX}{i:02d}' for i in range(1, 11)}
    for name in by_name:
        if name.startswith(PREFIX) and name not in names:
            errors.append(f'{name}: unexpected approach name')
    for index in range(1, 11):
        name = f'{PREFIX}{index:02d}'
        candidates = by_name.get(name, [])
        seats = by_name.get(f'STRE_FURN_PlayerSeat{index:02d}', [])
        if len(candidates) != 1:
            errors.append(f'{name}: expected one REFR, found {len(candidates)}')
            continue
        r = candidates[0]
        d = fields[r.offset]
        if r.signature != 'REFR' or r.form_id >> 24 != len(masters):
            errors.append(f'{name}: must be a plugin-owned REFR')
        if r.form_id in seen_ids:
            errors.append(f'{name}: duplicate FormID')
        seen_ids.add(r.form_id)
        if not r.flags & 0x400 or r.flags & (0x20 | 0x800):
            errors.append(f'{name}: must be persistent, enabled and not deleted')
        if d.get('NAME') != struct.pack('<I', base):
            errors.append(f'{name}: base must be Skyrim.esm XMarkerHeading')
        if len(seats) != 1 or seats[0].signature != 'REFR':
            errors.append(f'{name}: assigned Seat{index:02d} missing/ambiguous')
        elif cells.get(r.offset) != cell[0].form_id or cells.get(seats[0].offset) != cell[0].form_id:
            errors.append(f'{name}: marker/seat must belong to STRE_CELL_AlternateStart')
        if len(d.get('DATA', b'')) != 24:
            errors.append(f'{name}: missing reference transform')
            continue
        transform = struct.unpack('<6f', d['DATA'])
        if not all(math.isfinite(v) for v in transform) or abs(transform[3]) > 0.0001 or abs(transform[4]) > 0.0001:
            errors.append(f'{name}: nonfinite/tilted transform')
        if transform[:3] in seen_transforms:
            errors.append(f'{name}: duplicate approach position')
        seen_transforms.add(transform[:3])
        if d.get('XSCL', struct.pack('<f', 1.0)) != struct.pack('<f', 1.0):
            errors.append(f'{name}: expected scale 1')
        result.append(dict(rank=index - 1, editorId=name, localFormId=f'{r.form_id & 0xFFFFFF:08X}',
                           seatLocalFormId=f'{seats[0].form_id & 0xFFFFFF:08X}' if len(seats) == 1 else None,
                           position=transform[:3], rotation=transform[3:], cell=f'{cells.get(r.offset) or 0:08X}'))
    return result, errors


def check_bindings(rows: list[dict]) -> list[str]:
    errors = []
    for header, function, column in (
            ('StandingCreation.h', 'CreationMarkerLocalFormId', 'localFormId'),
            ('CreationSeating.h', 'CreationSeatLocalFormId', 'seatLocalFormId')):
        source = (ROOT / 'Code/common/CharacterCreation' / header).read_text(encoding='utf-8')
        table = source.split(function, 1)[1].split('};', 1)[0]
        bound = [int(v, 16) for v in re.findall(r'0x[0-9A-Fa-f]+', table)]
        if bound != [int(row[column], 16) for row in rows]:
            errors.append(f'{function}: C++ binding table differs from audited plugin')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plugin', type=Path, default=PLUGIN)
    args = parser.parse_args()
    try:
        data = args.plugin.read_bytes()
        rows, errors = audit(data)
        if not errors:
            errors.extend(check_bindings(rows))
        print(json.dumps(dict(plugin=str(args.plugin), sha256=hashlib.sha256(data).hexdigest(),
                              markers=rows, errors=errors, staticAudit='FAIL' if errors else 'PASS',
                              placementAndVisualAcceptance='human-required'), indent=2, allow_nan=False))
        return 1 if errors else 0
    except (AuditError, OSError, ValueError, struct.error) as exc:
        print(f'Audit failed: {exc}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
