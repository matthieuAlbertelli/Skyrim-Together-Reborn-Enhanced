"""Independent rejection tests using official xEdit's invented #96 fixture."""
from pathlib import Path
import struct
import unittest

from valen_helgen import Plugin, NARRATIVE, OWNER, ACCEPT_INFO
from valen_matrix_audit import audit

ROOT=Path(__file__).parent
BEFORE=(ROOT/'valen/fixtures/authored.esp').read_bytes()
AUTHORED=(ROOT/'valen/matrix/fixtures/authored.esp').read_bytes()
SINGLE='STRE_INFO_VALEN_SURVIVOR_SINGLE_KNOWN_FREED_ONLY_KNOWN_HADVAR_FREED'
CLASS='STRE_INFO_VALEN_MENU_RECRUIT_VALEN_CLASS_WARRIOR'


def edit(name, sig, transform, occurrence=0):
    """Corrupt fixed-size bytes only; never used to author production plugins."""
    plugin=Plugin(AUTHORED)
    record=next(r for r in plugin.records if r.form_id==plugin.ids[name])
    pos=record.offset+24
    end=pos+struct.unpack_from('<I',AUTHORED,record.offset+4)[0]
    seen=0
    while pos<end:
        tag,size=struct.unpack_from('<4sH',AUTHORED,pos)
        if tag.decode()==sig:
            if seen==occurrence:
                changed=transform(AUTHORED[pos+6:pos+6+size])
                assert len(changed)==size
                return AUTHORED[:pos+6]+changed+AUTHORED[pos+6+size:]
            seen+=1
        pos+=6+size
    raise AssertionError('Field not found: '+name+' '+sig)


class BinaryAuditTests(unittest.TestCase):
    def reject(self,data,pattern):
        with self.assertRaisesRegex(ValueError,pattern): audit(BEFORE,data)

    def test_official_output_and_actual_changes(self):
        report=audit(BEFORE,AUTHORED)
        self.assertEqual(report['actualChanges'],dict(created=293,updated=5,unchangedPlannedUpdates=2))
        self.assertEqual(report['coverage'],dict(automatic=32,explicit=17,incoherent=51,uncovered=0))
        self.assertEqual(len(report['records']),300)

    def test_technical_quest_is_immutable(self):
        self.reject(edit('STRE_QUEST_HelgenInvestigation','DNAM',lambda b:b'\x01'+b[1:]),'Unrelated record')

    def test_narrative_cannot_autostart(self):
        self.reject(edit(NARRATIVE,'DNAM',lambda b:b'\x01'+b[1:]),'Quest flags')

    def test_sidequest_type_is_preserved(self):
        self.reject(edit(NARRATIVE,'DNAM',lambda b:b[:8]+bytes(4)),'Quest flags')

    def test_discoveries_remain_two_independent_indices(self):
        self.reject(edit(NARRATIVE,'INDX',lambda b:struct.pack('<H',12)+b[2:],2),'Quest stages')

    def test_stages_cannot_complete_quest(self):
        self.reject(edit(NARRATIVE,'QSDT',lambda b:b'\x01',1),'cannot complete')

    def test_player_alias_is_forced_to_player(self):
        self.reject(edit(NARRATIVE,'ALFR',lambda b:bytes(4)),'player alias')

    def test_arrival_fragment_binding(self):
        self.reject(edit(NARRATIVE,'VMAD',lambda b:b.replace(b'Fragment_1',b'Fragment_0')),'fragment bindings')

    def test_observer_property_binding(self):
        self.reject(edit(NARRATIVE,'VMAD',lambda b:b.replace(struct.pack('<I',0x18A4A),bytes(4))),'Observer alias')

    def test_legacy_acceptance_fragment_is_immutable(self):
        self.reject(edit(ACCEPT_INFO,'VMAD',lambda b:b.replace(b'Fragment_0',b'Fragment_1')),'Acceptance VMAD')

    def test_state_reply_cannot_become_say_once(self):
        self.reject(edit(SINGLE,'ENAM',lambda b:bytes([b[0]|4])+b[1:]),'INFO flags')

    def test_no_or_bypass_of_discovery(self):
        self.reject(edit(SINGLE,'CTDA',lambda b:bytes([b[0]|1])+b[1:],2),'CTDA mismatch')

    def test_unknown_survivor_cannot_be_exposed_by_condition_change(self):
        self.reject(edit(SINGLE,'CTDA',lambda b:b[:4]+struct.pack('<f',1)+b[8:],3),'CTDA mismatch')

    def test_freed_and_departed_are_distinct(self):
        self.reject(edit(SINGLE,'CTDA',lambda b:b[:4]+struct.pack('<f',4)+b[8:],4),'CTDA mismatch')

    def test_condition_variable_identity(self):
        self.reject(edit(SINGLE,'CIS2',lambda b:b.replace(b'Hadvar',b'HadvaX')),'CTDA mismatch')

    def test_class_projection_stage(self):
        self.reject(edit(CLASS,'CTDA',lambda b:b[:16]+struct.pack('<I',101)+b[20:],1),'CTDA mismatch')

    def test_graph_links_are_exact(self):
        self.reject(edit(SINGLE,'TCLT',lambda b:bytes(4)),'Graph link')

    def test_french_text_is_frozen(self):
        self.reject(edit(SINGLE,'NAM1',lambda b:b'X'+b[1:]),'French response')

    def test_stage_log_association_cannot_disappear(self):
        self.reject(AUTHORED.replace(b'QSDT',b'ALED',1),'Field order/multiplicity')

    def test_truncation(self):
        with self.assertRaises(Exception): audit(BEFORE,AUTHORED[:-1])


if __name__=='__main__': unittest.main()
