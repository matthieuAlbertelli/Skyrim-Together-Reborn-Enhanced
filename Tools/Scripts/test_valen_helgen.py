"""Independent audit regressions against real xEdit output on synthetic input."""
import struct
import tempfile
import unittest
import wave
from pathlib import Path

import valen_helgen as v


FIXTURES = Path(__file__).parent / 'valen/fixtures'
BEFORE = (FIXTURES / 'before.esp').read_bytes()
AUTHORED = (FIXTURES / 'authored.esp').read_bytes()


def edit_field(name, sig, transform, occurrence=0):
    """Mutate a field in-place, preserving all container sizes."""
    p = v.Plugin(AUTHORED)
    record = next(r for r in p.records if r.form_id == p.ids[name])
    start = record.offset + 24
    pos, seen = start, 0
    while pos < start + struct.unpack_from('<I', AUTHORED, record.offset + 4)[0]:
        field, size = struct.unpack_from('<4sH', AUTHORED, pos)
        if field.decode() == sig:
            if seen == occurrence:
                changed = transform(AUTHORED[pos + 6:pos + 6 + size])
                assert len(changed) == size
                return AUTHORED[:pos + 6] + changed + AUTHORED[pos + 6 + size:]
            seen += 1
        pos += 6 + size
    raise AssertionError('Field not found')


class ValenAuditTests(unittest.TestCase):
    def reject(self, data, pattern):
        with self.assertRaisesRegex(ValueError, pattern):
            v.audit(BEFORE, data)

    def test_official_xedit_synthetic_output(self):
        result = v.audit(BEFORE, AUTHORED)['result']
        self.assertEqual(len(result['Records']), 7)
        self.assertEqual(result['FormKey'], '000802:STRE_AlternateStart.esp')

    def test_existing_technical_quest_cannot_change(self):
        self.reject(edit_field('STRE_QUEST_HelgenInvestigation', 'DNAM', lambda b: b'\x01' + b[1:]), 'Existing payload')

    def test_narrative_cannot_autostart(self):
        self.reject(edit_field(v.NARRATIVE, 'DNAM', lambda b: b'\x01' + b[1:]), 'Quest flags')

    def test_stage_cannot_complete(self):
        self.reject(edit_field(v.NARRATIVE, 'QSDT', lambda b: b'\x01'), 'uncompleted')

    def test_wrong_valen_condition(self):
        self.reject(edit_field(v.ACCEPT_INFO, 'CTDA', lambda b: b[:12] + bytes(4) + b[16:]), 'condition')

    def test_or_condition_cannot_bypass_acceptance(self):
        self.reject(edit_field(v.ACCEPT_INFO, 'CTDA', lambda b: bytes([b[0] | 1]) + b[1:]), 'condition')

    def test_stage_ten_cannot_remain_eligible(self):
        self.reject(edit_field(v.ACCEPT_INFO, 'CTDA', lambda b: b[:4] + struct.pack('<f', 11) + b[8:], 1), 'condition')

    def test_wrong_topic_link(self):
        self.reject(edit_field(v.NEWS_INFO, 'TCLT', lambda b: bytes(4)), 'continuation')

    def test_wrong_info_parent(self):
        data = bytearray(AUTHORED)
        pos = data.index(b'GRUP', data.index(v.NEWS.encode()))
        struct.pack_into('<I', data, pos + 8, 0)
        self.reject(bytes(data), 'parent/order')

    def test_wrong_vmad_quest_binding(self):
        self.reject(edit_field(v.ACCEPT_INFO, 'VMAD', lambda b: b.replace(struct.pack('<I', 0x03000802), struct.pack('<I', 0x03000801))), 'binding')

    def test_french_accent_corruption(self):
        self.reject(edit_field(v.ACCEPT_INFO, 'NAM1', lambda b: b.replace(b'\xea', b'e')), 'French')

    def test_unexpected_field_is_rejected(self):
        self.reject(AUTHORED.replace(b'QOBJ', b'QSTA'), 'field/order')

    def test_truncation_is_rejected(self):
        with self.assertRaises(Exception):
            v.audit(BEFORE, AUTHORED[:-1])

    def test_generated_companions_match_allocated_identity(self):
        report = v.audit(BEFORE, AUTHORED)
        with tempfile.TemporaryDirectory() as tmp:
            data = Path(tmp) / 'modding/fixture/Data'
            v.generate(report, data, 'MaleWarlock')
            self.assertEqual((data / 'SEQ/STRE_AlternateStart.seq').read_bytes(), struct.pack('<I', 0x03000803))
            for kind in ('QF', 'TIF'):
                name = report['result'][kind]
                self.assertIn('Scriptname ' + name, (data / f'Source/Scripts/{name}.psc').read_text())
            audio = sorted(data.rglob('*.wav'))
            self.assertEqual([p.name for p in audio], ['stre_quest_stre_dial_valen_00000807_1.wav',
                             'stre_quest_stre_dial_valen_00000807_2.wav', 'stre_quest_stre_dial_valen_00000808_1.wav'])
            for path, seconds in zip(audio, (9, 7, 10)):
                with wave.open(str(path)) as wav:
                    self.assertEqual(wav.getnframes(), 44100 * seconds)
                    self.assertEqual(wav.readframes(wav.getnframes()), bytes(88200 * seconds))

    def test_generation_refuses_production_path(self):
        with self.assertRaisesRegex(ValueError, 'private'):
            v.generate(v.audit(BEFORE, AUTHORED), Path('GameFiles/Skyrim'), 'MaleWarlock')


if __name__ == '__main__':
    unittest.main()
