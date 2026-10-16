"""Presentation regression tests; binary mutations use invented fixtures only."""
from collections import Counter
from pathlib import Path
import struct
import unittest
from unittest.mock import patch

import valen_presentation as v
from valen_helgen import Plugin

FIXTURE = (Path(__file__).parent / 'valen/matrix/fixtures/authored.esp').read_bytes()


def fixture_edit(edits):
    """Rebuild only the hardcoded synthetic fixture, never a caller's ESP."""
    p = Plugin(FIXTURE)
    edits = {(p.ids[name], tag, index): value for (name, tag, index), value in edits.items()}

    def walk(start, end):
        result = bytearray()
        pos = start
        while pos < end:
            size = struct.unpack_from('<I', FIXTURE, pos+4)[0]
            header = bytearray(FIXTURE[pos:pos+24])
            if header[:4] == b'GRUP':
                body = walk(pos+24, pos+size)
                struct.pack_into('<I', header, 4, 24+len(body))
                pos += size
            else:
                fid = struct.unpack_from('<I', header, 12)[0]
                counts = Counter()
                body = bytearray()
                for tag, raw in p.fields[fid]:
                    raw = edits.get((fid, tag, counts[tag]), raw)
                    counts[tag] += 1
                    body += tag.encode() + struct.pack('<H', len(raw)) + raw
                struct.pack_into('<I', header, 4, len(body))
                pos += 24+size
            result += header + body
        return bytes(result)
    return walk(0, len(FIXTURE))


def projected(variant, extra=None):
    edits = {}
    p = Plugin(FIXTURE)
    for c in v.changes(FIXTURE, variant):
        value = (struct.pack('<H', c['after'])+p.one(p.ids[c['edid']], 'ENAM')[2:]
                 if c['field'] == 'ENAM' else c['after'].encode('cp1252')+b'\0')
        edits[c['edid'], c['field'], c['index']] = value
    edits.update(extra or {})
    return fixture_edit(edits)


class PresentationTests(unittest.TestCase):
    def test_exact_six_substitutions_and_preserved_french(self):
        self.assertEqual(v.runtime_text('’‘“”—…'), "''\"\"-...")
        keep = 'é è à ê â ô û ù î ï ç É À Ê « » - \' " ...\n\t'
        self.assertEqual(v.runtime_text(keep), keep)
        self.assertEqual(v.runtime_text(v.runtime_text('d’être…')), "d'être...")

    def test_corpus_and_fingerprint_stay_frozen(self):
        before = v.corpus.CORPUS.read_bytes()
        v.render(FIXTURE, 'B')
        v.frozen_corpus()
        self.assertEqual(v.corpus.CORPUS.read_bytes(), before)
        with patch.object(v, 'CORPUS_SHA256', '0'*64), self.assertRaisesRegex(ValueError, 'corpus bytes'):
            v.frozen_corpus()
        with patch.object(v, 'EDITORIAL_SHA256', '0'*64), self.assertRaisesRegex(ValueError, 'fingerprint'):
            v.frozen_corpus()

    def test_a_b_are_identical_except_one_no_lip_bit(self):
        a, b = projected('A'), projected('B')
        self.assertEqual(sum(x != y for x,y in zip(a,b)), 1)
        self.assertEqual(len(a), len(b))
        for variant, blob in [('A',a), ('B',b)]:
            report = v.audit(FIXTURE, blob, variant)
            self.assertTrue(report['success'])
            self.assertEqual(report['flagChanges'], int(variant=='B'))
            self.assertEqual(report['checkedFields'], dict(FULL=121, RNAM=2, NAM1=671, NNAM=2))
        flags = Plugin(b).integer(Plugin(b).ids[v.TRIAL_INFO], 'ENAM')
        self.assertEqual(flags, 0x0200)  # Force Subtitle per xEdit/Mutagen enum.

    def test_unsafe_baseline_is_not_a_typography_pass(self):
        with self.assertRaisesRegex(ValueError, 'Unexpected serialized delta'):
            v.audit(FIXTURE, FIXTURE, 'A')

    def test_accent_loss_and_unapproved_flags_are_rejected(self):
        for edits in [
            {(v.TRIAL_INFO,'NAM1',0): b'Un barde, si vous tenez aux reponses courtes.\0'},
            {(v.TRIAL_INFO,'ENAM',0): bytes(4)},
            {('STRE_INFO_ValenHelgen_News','ENAM',0): struct.pack('<HH',0x200,0)},
            {(v.NARRATIVE,'NNAM',1): 'Enquêter…\0'.encode('cp1252')},
            {('STRE_INFO_VALEN_MENU_HELGEN_NAV_0','RNAM',0): 'Qu’est-ce ?\0'.encode('cp1252')},
            {('STRE_DIAL_VALEN_SELF_010','FULL',0): 'Qui êtes-vous…\0'.encode('cp1252')},
        ]:
            with self.subTest(edits=edits), self.assertRaisesRegex(ValueError,'Unexpected serialized delta'):
                v.audit(FIXTURE, projected('B', edits), 'B')

    def test_authoring_has_no_record_allocation_or_asset_generation(self):
        script = v.render(FIXTURE, 'B')
        self.assertIn("SetElementNativeValues(e,'ENAM\\Flags',512)", script)
        self.assertEqual(script.count("SetElementNativeValues(e,'ENAM\\Flags'"),1)
        for forbidden in ('SetFormID', 'SetLoadOrderFormID', 'Add(parent', 'Add(target', 'Wave', 'Papyrus'):
            self.assertNotIn(forbidden, script)
        with self.assertRaisesRegex(ValueError, 'Only A/B'):
            v.changes(FIXTURE, 'C')

    def test_filename_matches_xedit_quest_topic_budget(self):
        self.assertEqual(v.voice_filename(v.OWNER,'STRE_DIAL_VALEN_SELF_010',0x030D6BC3,1),
                         'stre_quest_stre_dial_valen_000d6bc3_1.wav')
        self.assertEqual(v.voice_filename('Q','T'*30,0xFE123456,2),'q_'+'t'*24+'_00123456_2.wav')
        self.assertEqual(v.voice_filename('Quest','Topic',123,1),'quest_topic_0000007b_1.wav')


if __name__ == '__main__':
    unittest.main()
