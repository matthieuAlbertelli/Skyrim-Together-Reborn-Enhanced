"""#96 bounded plan, routing, lifecycle and independent candidate checks."""
from copy import deepcopy
from pathlib import Path
import itertools
import re
import subprocess
import sys
import tempfile
import unittest

import valen_matrix as corpus
from valen_matrix_plan import build_plan, OWNER, NARRATIVE, GENERIC
from valen_matrix_writer import render
from valen_matrix_audit import audit, evaluate, FUNCTIONS
from valen_helgen import Plugin

DATA=corpus.read_json(corpus.CORPUS)
PLAN=build_plan(DATA)
ROOT=corpus.ROOT


class AuthoringPlanTests(unittest.TestCase):
    def test_generic_info_chain_follows_approved_specificity_order(self):
        actual=[r['edid'] for r in PLAN['records'] if r.get('topic')==GENERIC and '_NAV_' not in r['edid']]
        expected=['STRE_INFO_'+r['node']+'_'+r['id'].replace('-','_').upper()
                  for r in DATA['survivors']['rules'] if
                  next(n for n in DATA['nodes'] if n['id']==r['node']).get('route')=='generic-survivor-topic']
        self.assertEqual(actual,expected)

    def test_both_freed_table_names_the_actual_explicit_reply(self):
        row=next(r for r in corpus.survivor_rows(DATA) if
                 (r['HadvarDiscovered'],r['RalofDiscovered'],r['HadvarState'],r['RalofState'])==(1,1,3,3))
        self.assertEqual(row['result'],'explicit')
        self.assertEqual(row['explicitReport'],'VALEN_SURVIVOR_BOTH_SAFE')

    def test_real_xedit_sorted_array_failure_is_rejected(self):
        before=(ROOT/'Tools/Scripts/valen/fixtures/authored.esp').read_bytes()
        rejected=(ROOT/'Tools/Scripts/valen/matrix/fixtures/rejected-sorted-array.esp').read_bytes()
        with self.assertRaisesRegex(ValueError,'Quest stages'):
            audit(before,rejected)

    def test_synthetic_masters_do_not_redefine_xedit_builtin_player(self):
        # The former ACHR 00000014 collided with xEdit's built-in PLYR before
        # Initialize could run. This fixture must remain loadable and invented.
        with tempfile.TemporaryDirectory() as tmp:
            subprocess.run([sys.executable, str(ROOT/'Tools/Scripts/valen/matrix/fixture.py'), tmp],
                           check=True, capture_output=True)
            for name in ('Skyrim.esm','Update.esm','Dragonborn.esm'):
                plugin=Plugin((Path(tmp)/'Data'/name).read_bytes())
                self.assertNotIn(0x14,plugin.fields)
                self.assertFalse({'ACHR','NPC_','CELL'} & {r.signature for r in plugin.records})

    def test_bounded_records_and_legacy_identity_allowlist(self):
        self.assertEqual(PLAN['counts'],{'QUST':2,'DLBR':17,'DIAL':121,'INFO':160})
        self.assertEqual(PLAN['actions'],{'update':7,'create':293})
        self.assertEqual({r['edid'] for r in PLAN['records'] if r['action']=='update'},
                         {'STRE_QUEST_ValenHelgen','STRE_QUEST_ValenDialogue','STRE_DLBR_ValenHelgen',
                          'STRE_DIAL_ValenHelgen_News','STRE_DIAL_ValenHelgen_Accept',
                          'STRE_INFO_ValenHelgen_News','STRE_INFO_ValenHelgen_Accept'})
        self.assertFalse(any('ALDUIN' in r['edid'] for r in PLAN['records']))

    def test_every_approved_response_is_represented_without_editorial_changes(self):
        planned={rid for r in PLAN['records'] for rid in r.get('responseIds',[])}
        expected={r['id'] for n in DATA['nodes'] for r in n['responses']}
        expected.update(n['leadIn']['id'] for n in DATA['nodes'] if 'leadIn' in n)
        self.assertEqual(planned,expected)
        self.assertEqual(corpus.editorial_hash(DATA),'9B29AFABA5E75CF9436817AA77619B3408DB9530D5884F896E05C94EA122D4F5')
        for record in PLAN['records']:
            if record['signature']=='INFO' and record['action']=='create':
                self.assertLessEqual(len(record['segments']),255)
                self.assertTrue(all(len(s.encode('cp1252'))<=255 for s in record['segments']))

    def test_generic_rules_are_exclusive_in_all_100_contexts(self):
        generic=[r for r in PLAN['records'] if r.get('topic')==GENERIC]
        for row in corpus.survivor_rows(DATA):
            def context(c):
                if c['function']==72: return 1
                if c['function']==58: return 20
                if c['function']==59: return row['HadvarDiscovered'] if c['argument']==11 else row['RalofDiscovered']
                return row['HadvarState'] if c['argument']=='::HadvarState_var' else row['RalofState']
            selected=[]
            for r in generic:
                conditions=[dict(c,function=FUNCTIONS[c['function']]) for c in r['conditions']]
                if evaluate(conditions,context): selected.append(r['node'])
            self.assertEqual(selected,[row['genericINFO']] if row['result']=='automatic' else [])

    def test_only_exit_has_goodbye_and_only_existing_acceptance_has_effect(self):
        infos=[r for r in PLAN['records'] if r['signature']=='INFO']
        self.assertEqual([r['node'] for r in infos if r['flags']&1],['VALEN_EXIT'])
        self.assertEqual([r['node'] for r in infos if r['fragment']],['VALEN_HELGEN_ACCEPT'])
        for r in infos:
            if r['node'].startswith('VALEN_SURVIVOR_SINGLE_KNOWN_'):
                self.assertEqual(r['flags'],0xA00)
                self.assertEqual(r['topic'],GENERIC)

    def test_class_stages_match_native_mapping_and_are_local(self):
        source=(ROOT/'Code/common/CharacterCreation/ValenDialogueProjection.h').read_text('utf-8')
        self.assertEqual(re.findall(r'"(class\.[a-z]+)"',source),[c['id'] for c in DATA['classes']])
        owner=next(r for r in PLAN['records'] if r['edid']==OWNER)
        self.assertEqual(owner['stages'],[0,50]+list(range(100,122)))
        cpp=(ROOT/'Code/client/Services/Generic/CharacterCreationService.cpp').read_text('utf-8')
        self.assertEqual(cpp.count('ProjectValenDialogueClass();'),1)
        finalize=cpp.split('void CharacterCreationService::FinalizeCompletedBuild()')[1].split('void CharacterCreationService::ProjectValenDialogueClass()')[0]
        self.assertIn('!m_buildConfirmed',finalize)
        self.assertIn('ProjectValenDialogueClass();',finalize)
        sync=(ROOT/'Code/client/Services/Generic/QuestService.cpp').read_text('utf-8')
        excluded=sync.split('kNonSyncableQuestEditorIds')[1].split('};')[0]
        self.assertIn('"STRE_QUEST_ValenDialogue"',excluded)
        self.assertNotIn('"STRE_QUEST_ValenHelgen"',excluded)

    def test_writer_allocates_only_through_xedit_and_private_output(self):
        script=render(PLAN)
        self.assertNotIn('{ GENERATED_PLAN }',script)
        self.assertIn("Add(parent,'INFO',True)",script)
        self.assertIn("ExportPrivate('authored.esp')",script)
        self.assertNotIn('SetLoadOrderFormID',script)
        self.assertNotIn('SetFormID',script)
        self.assertNotIn('SkyrimTogether.esp',script)
        self.assertNotIn('SSEScript.exe',script)

    def test_observer_has_both_projection_states_and_recovery_fences(self):
        psc=(ROOT/'Tools/Scripts/valen/matrix/STRE_ValenHelgenObserver.psc').read_text('utf-8')
        for required in ('OnPlayerLoadGame','OnLocationChange','RegisterForSingleUpdate','IsHelgenCampaignRequired',
                         'IsHelgenInvestigationStartAuthorized','IsDisabled','IsDead','Is3DLoaded','GetParentCell',
                         'IsInLocation','<= 256.0','HasLOS','physicalState == 1 || physicalState == 2',
                         'physicalState == 3','physicalState == 4','GetStageDone(20)','GetStageDone(stage)'):
            self.assertIn(required,psc)
        for forbidden in ('MoveTo(', 'Kill(', 'Resurrect(', 'Reset(', 'StartQuest', 'MainQuestPath =', 'HadvarState =', 'RalofState ='):
            self.assertNotIn(forbidden,psc)
        self.assertLess(psc.index('narrative.SetStage(20)'),psc.index('narrative.SetStage(stage)'))


if __name__=='__main__':
    unittest.main()
