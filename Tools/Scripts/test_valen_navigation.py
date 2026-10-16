"""Topology rejection tests on a fixed invented, official-xEdit fixture only."""
from pathlib import Path
import struct
import unittest
from copy import deepcopy

import valen_navigation as v

FIXTURES=Path(__file__).parent/'valen/navigation/fixtures'
BEFORE=(FIXTURES/'before.esp').read_bytes()
AUTHORED=(FIXTURES/'authored.esp').read_bytes()
SELF='STRE_INFO_VALEN_MENU_SELF_R'
NAV0='STRE_INFO_VALEN_MENU_HELGEN_NAV_0'
NAV2='STRE_INFO_VALEN_MENU_HELGEN_NAV_2'


def edit(name, tag, replacement, occurrence=0):
    """Hardcoded synthetic bytes; cannot accept a game plugin."""
    p=v.Plugin(AUTHORED); target=p.ids[name] if name else 0
    def walk(start,end):
        result=bytearray(); pos=start
        while pos<end:
            header=bytearray(AUTHORED[pos:pos+24]); size=struct.unpack_from('<I',header,4)[0]
            if header[:4]==b'GRUP':
                body=walk(pos+24,pos+size); struct.pack_into('<I',header,4,len(body)+24); pos+=size
            else:
                fid=struct.unpack_from('<I',header,12)[0]; body=bytearray(); seen=0
                for s,raw in p.fields[fid]:
                    if fid==target and s==tag:
                        if seen==occurrence:raw=replacement(raw) if callable(replacement) else replacement
                        seen+=1
                    if raw is not None:body+=s.encode()+struct.pack('<H',len(raw))+raw
                struct.pack_into('<I',header,4,len(body)); pos+=size+24
            result+=header+body
        return bytes(result)
    return walk(0,len(AUTHORED))


class NavigationTests(unittest.TestCase):
    def reject(self,blob):
        with self.assertRaises(ValueError):v.audit(BEFORE,blob,fixture=True)

    def test_official_serialization_and_eligibility(self):
        report=v.audit(BEFORE,AUTHORED,fixture=True)
        self.assertEqual((report['rootCategories'],report['navigationInfos']),(11,13))
        self.assertEqual((report['classContexts'],report['survivorContexts']),(22,100))
        self.assertEqual(report['engineRuntime'],'NOT_TESTED')
        self.assertEqual(len(report['created']),4)

    def test_zero_response_baseline_is_rejected(self):
        with self.assertRaises(ValueError):
            v.audit_topology(v.Plugin(BEFORE),v.build_navigation_plan(v.frozen_corpus()))

    def test_responses_are_required_without_fake_subtitle(self):
        for tag,value in [('TRDT',None),('NAM1',None),('NAM1',b'\0'),('NAM1',b'Oui.\0'),
                          ('TRDT',bytes(24)),('ENAM',struct.pack('<HH',0xA00,0)),
                          ('ENAM',struct.pack('<HH',0x840,0)),('ENAM',struct.pack('<HH',0x804,0))]:
            with self.subTest(tag=tag,value=value):self.reject(edit(SELF,tag,value))

    def test_starting_topic_and_internal_top_level_rejected(self):
        self.reject(edit('STRE_DLBR_VALEN_MENU_SELF','SNAM',bytes(4)))
        self.reject(edit('STRE_DLBR_VALEN_SURVIVOR_HADVAR','DNAM',struct.pack('<I',1)))
        self.reject(edit('STRE_DLBR_VALEN_MENU_SELF','DNAM',bytes(4)))

    def test_exact_helgen_stage_partition_and_prompt(self):
        self.reject(edit(NAV0,'CTDA',lambda b:b[:4]+struct.pack('<f',10)+b[8:],2))
        self.reject(edit(NAV2,'CTDA',lambda b:b[:4]+struct.pack('<f',19)+b[8:],1))
        self.reject(edit(NAV2,'RNAM',b'Helgen\0'))

    def test_links_and_previous_info_cannot_drift(self):
        self.reject(edit(SELF,'TCLT',bytes(4)))
        self.reject(edit(v.RETURN_INFO,'TCLT',None))
        self.reject(edit(NAV2,'PNAM',bytes(4)))
        self.reject(edit(v.HELGEN_MENU,'TIFC',bytes(4)))

    def test_recruit_is_repeatable_and_conditions_are_frozen(self):
        name='STRE_INFO_VALEN_MENU_RECRUIT_VALEN_CLASS_MAGE'
        self.reject(edit(name,'ENAM',struct.pack('<HH',0xA04,0)))
        self.reject(edit(name,'CTDA',lambda b:b[:4]+bytes(4)+b[8:],1))

    def test_narrative_and_header_immutable(self):
        self.reject(edit('STRE_INFO_VALEN_SELF_010_R','NAM1',b'Texte modifie\0'))
        self.reject(edit('STRE_INFO_ValenHelgen_Accept','VMAD',lambda b:b.replace(b'Fragment_0',b'Fragment_1')))
        self.reject(edit(v.NARRATIVE,'NNAM',b'Autre objectif\0',1))
        self.reject(edit(None,'MAST',b'Other.esm\0'))

    def test_no_corpus_asset_or_native_gate_changes(self):
        data=v.frozen_corpus(); original=deepcopy(data)
        plan=v.build_navigation_plan(data); script=v.render()
        self.assertEqual(original,data)
        self.assertEqual(plan['navigationResponse']['voiceAssets'],[])
        self.assertNotIn('Invisible Continue',script)
        self.assertNotIn('SetLoadOrderFormID',script)
        roots=set(plan['rootTopics'].values())
        for r in plan['records']:
            if r['signature']!='INFO':continue
            self.assertEqual(set(r['links'])&roots,roots if r['edid']==v.RETURN_INFO else set())
        # Root return is an adapter prompt, never a new canonical NPC sentence.
        self.assertNotIn('ADAPTER_RETURN_ROOT',{n['id'] for n in data['nodes']})

    def test_production_cannot_accept_fixture_baseline(self):
        with self.assertRaisesRegex(ValueError,'baseline'):v.audit(BEFORE,AUTHORED)

    def test_manifest_contains_only_four_new_adapter_identities(self):
        original={'pluginName':'STRE_AlternateStart.esp','records':[],'notes':['unchanged']}
        output=v.manifest_for(original,AUTHORED)
        self.assertEqual(len(output['records']),4)
        self.assertEqual(output['notes'],original['notes'])
        self.assertEqual(original['records'],[])


if __name__=='__main__':unittest.main()
