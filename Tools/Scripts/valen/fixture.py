"""Synthetic xEdit input for #94 integration tests; never reads game content."""
from pathlib import Path
import struct


def sub(sig, value):
    return sig.encode() + struct.pack('<H', len(value)) + value


def record(sig, fid, payload, flags=0):
    return struct.pack('<4sIIIIHH', sig.encode(), len(payload), flags, fid, 0, 44, 0) + payload


def group(sig, records):
    return struct.pack('<4sI4sIII', b'GRUP', len(records) + 24, sig.encode(), 0, 0, 0) + records


def header(masters, count=0, flags=0):
    data = sub('HEDR', struct.pack('<fII', 1.7, count, 0x800))
    for master in masters:
        data += sub('MAST', master.encode() + b'\0') + sub('DATA', bytes(8))
    return record('TES4', 0, data, flags)


def create(root: Path):
    data = root / 'Data'
    data.mkdir(parents=True)
    voice = record('VTYP', 0x9843A, sub('EDID', b'MaleWarlock\0') + sub('DNAM', b'\0'))
    (data / 'Skyrim.esm').write_bytes(header([], 1, 1) + group('VTYP', voice))
    (data / 'Update.esm').write_bytes(header(['Skyrim.esm'], flags=1))
    (data / 'Dragonborn.esm').write_bytes(header(['Skyrim.esm', 'Update.esm'], flags=1))
    npc = record('NPC_', 0x03000800, sub('EDID', b'STRE_NPC_MessireValen\0') +
        sub('OBND', bytes(12)) + sub('ACBS', bytes(24)) + sub('VTCK', struct.pack('<I', 0x9843A)) +
        sub('RNAM', bytes(4)) + sub('AIDT', bytes(20)) + sub('CNAM', bytes(4)) + sub('DATA', b'') +
        sub('NAM5', b'') + sub('NAM6', bytes(4)) + sub('NAM7', bytes(4)) + sub('NAM8', bytes(4)))
    quest = record('QUST', 0x03000801, sub('EDID', b'STRE_QUEST_HelgenInvestigation\0') +
        sub('DNAM', bytes(12)) + sub('NEXT', b'') + sub('ANAM', bytes(4)))
    baseline = header(['Skyrim.esm', 'Update.esm', 'Dragonborn.esm'], 2) + group('NPC_', npc) + group('QUST', quest)
    (data / 'STRE_AlternateStart.esp').write_bytes(baseline)
    (root / 'before.esp').write_bytes(baseline)


if __name__ == '__main__':
    import sys
    create(Path(sys.argv[1]))
