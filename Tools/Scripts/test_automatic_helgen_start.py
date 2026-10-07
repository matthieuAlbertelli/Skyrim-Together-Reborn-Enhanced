"""ESP fragment metadata and native/Papyrus integration contracts (not gameplay)."""
import re
import struct
import unittest
from pathlib import Path
from audit_mq101_quickstart5 import parse_records, record_payload, iter_subrecords
from test_remote_respawn_lab import body

ROOT = Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")

ALT = read("GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc")
INV = read("GameFiles/Skyrim/Source/Scripts/STRE_HelgenInvestigationController.psc")
CREATION = read("Code/client/Services/Generic/CharacterCreationService.cpp")
SERVER = read("Code/server/Services/CharacterBuildService.cpp")

class QuestVmad:
    """Read Skyrim VMAD v5/object-format 2 through the QUST fragment table."""
    def __init__(self, data):
        self.data, self.pos = data, 0
    def take(self, count):
        value = self.data[self.pos:self.pos + count]
        if len(value) != count:
            raise ValueError("truncated VMAD")
        self.pos += count
        return value
    def number(self, fmt):
        return struct.unpack('<' + fmt, self.take(struct.calcsize('<' + fmt)))[0]
    def string(self):
        return self.take(self.number('H')).decode('utf-8')
    def value(self, kind):
        if kind >= 11:
            for _ in range(self.number('I')):
                self.value(kind - 10)
        elif kind == 2:
            self.string()
        else:
            self.take({1: 8, 3: 4, 4: 4, 5: 1}[kind])
    def fragments(self):
        assert self.number('H') == 5
        assert self.number('H') == 2
        for _ in range(self.number('H')):
            self.string()
            self.take(1)
            for _ in range(self.number('H')):
                self.string()
                kind = self.number('B')
                self.take(1)
                self.value(kind)
        self.take(1)
        count = self.number('H')
        self.string()
        result = {}
        for _ in range(count):
            stage = self.number('H')
            self.take(2)
            index = self.number('I')
            self.take(1)
            script, fragment = self.string(), self.string()
            if (stage, index) in result:
                raise ValueError('duplicate stage fragment')
            result[stage, index] = (script, fragment)
        return result

class AutomaticHelgenStart(unittest.TestCase):
    def test_campaign_required_clears_on_runtime_end_and_accepted_leave(self):
        service = read('Code/client/Services/Generic/CampaignService.cpp')
        menu = body(service, 'void CampaignService::OnMainMenuEntered(')
        # Must also run when Disconnect has already removed the admission.
        self.assertLess(menu.index('EndRuntimeSession()'), menu.index('m_helgenCampaignRequired.store(false)'))
        self.assertLess(menu.index('m_helgenCampaignRequired.store(false)'), menu.index('return;'))
        self.assertNotIn('if (!admission)', menu)
        response = body(service, 'void CampaignService::OnCommandResponse(')
        rejected = body(response, 'if (!Succeeded(acResponse.Result))')
        self.assertIn('return;', rejected)
        self.assertNotIn('m_helgenCampaignRequired', rejected)
        leave = body(response, 'else if (acResponse.Operation == CampaignProtocolOperation::Leave)')
        self.assertLess(leave.index('m_admissionState.Leave('), leave.index('m_helgenCampaignRequired.store(false)'))
        self.assertIn('if (!m_admissionState.GetAdmission())\n            m_helgenCampaignRequired.store(false);', leave)

    def test_disconnect_preserves_campaign_fence_and_resume_reasserts_it(self):
        service = read('Code/client/Services/Generic/CampaignService.cpp')
        disconnect = body(service, 'void CampaignService::OnDisconnected(')
        self.assertIn('m_admissionState.Disconnect()', disconnect)
        self.assertNotIn('EndRuntimeSession()', disconnect)
        for marker in ('void CampaignService::OnDisconnected(', 'void CampaignService::ClearVolatileProjection('):
            self.assertNotIn('m_helgenCampaignRequired', body(service, marker))
        response = body(service, 'void CampaignService::OnCommandResponse(')
        acceptance = body(response, 'if ((acResponse.Operation == CampaignProtocolOperation::Create')
        self.assertLess(acceptance.index('m_admissionState.Accept('), acceptance.index('m_helgenCampaignRequired.store(true)'))
        self.assertIn('acResponse.Operation == CampaignProtocolOperation::Resume) &&', response)
        # The saved campaign fence must survive a menu/Continue in the same process.
        self.assertEqual(INV.count('MultiplayerCampaignObserved = False'), 1)  # declaration only
        self.assertIn('If MultiplayerCampaignObserved && !SkyrimTogetherUtils.IsHelgenInvestigationStartAuthorized()', INV)
        self.assertIn('If MultiplayerCampaignObserved || SkyrimTogetherUtils.IsHelgenCampaignRequired()', INV)

    def test_cleanup_start_failure_remains_retryable_before_mq101(self):
        helper = ALT.split('Bool Function EnsurePostHelgenProjection()', 1)[1]
        claim = helper.index('PostHelgenProjectionStarted = True')
        first_mutation = helper.index('MQ101.SetStage(20)')
        startup = re.search(r'If !STREHelgenNPCCleanup.IsRunning\(\)\s+If !STREHelgenNPCCleanup.Start\(\)\s+Debug.Trace\([^\n]+\)\s+Return False\s+EndIf\s+EndIf', helper)
        self.assertIsNotNone(startup)
        self.assertLess(startup.end(), claim)
        self.assertLess(claim, first_mutation)
        self.assertNotIn('PostHelgenProjectionStarted =', helper[:startup.end()])
        self.assertNotIn('MQ101.SetStage', helper[:claim])

    def test_partial_projection_still_rejects_replay_including_latent_start_race(self):
        helper = ALT.split('Bool Function EnsurePostHelgenProjection()', 1)[1]
        startup = helper.index('STREHelgenNPCCleanup.Start()')
        claim = helper.index('PostHelgenProjectionStarted = True')
        guards = list(re.finditer(r'If PostHelgenProjectionStarted\s+(?:Debug.Trace\([^\n]+\)\s+)?Return False\s+EndIf', helper))
        self.assertEqual(len(guards), 2)
        self.assertLess(guards[0].end(), startup)
        self.assertLess(startup, guards[1].start())
        self.assertLess(guards[1].end(), claim)
        self.assertEqual(helper.count('PostHelgenProjectionStarted = True'), 1)
        self.assertNotIn('PostHelgenProjectionStarted = False', helper)
        self.assertIn('PostHelgenProjectionCompleted = True\n    Return True\nEndFunction', helper)

    def test_real_esp_stage_fragment_mapping_and_startup_flags(self):
        data = (ROOT / 'GameFiles/Skyrim/STRE_AlternateStart.esp').read_bytes()
        quests = {}
        for record in parse_records(data):
            if record.signature != 'QUST':
                continue
            subs = list(iter_subrecords(record_payload(data, record)))
            edid = next((v.rstrip(b'\0').decode() for k, v in subs if k == 'EDID'), '')
            if edid in ('STRE_QUEST_AlternateStart', 'STRE_QUEST_HelgenInvestigation'):
                quests[edid] = subs
        alt = quests['STRE_QUEST_AlternateStart']
        fragments = QuestVmad(dict(alt)['VMAD']).fragments()
        self.assertEqual({(s, i): f for (s, i), (_, f) in fragments.items()},
                         {(10, 0): 'Fragment_0', (11, 0): 'Fragment_4', (30, 0): 'Fragment_6'})
        stages = {struct.unpack_from('<H', v)[0]: v[2] for k, v in alt if k == 'INDX'}
        self.assertEqual(stages[10] & 2, 2)  # Startup stage invokes BeginCharacterCreation.
        self.assertIn(0, stages)
        self.assertIn(20, stages)
        inv = QuestVmad(dict(quests['STRE_QUEST_HelgenInvestigation'])['VMAD']).fragments()
        self.assertEqual({(s, i): f for (s, i), (_, f) in inv.items()}, {(10, 0): 'Fragment_0'})

    def test_stopped_alternate_start_is_never_restarted_by_adapter(self):
        finalize = body(CREATION, 'void CharacterCreationService::FinalizeCompletedBuild(')
        self.assertIn('m_pQuest->SetStopped()', finalize)
        projection = body(CREATION, 'void CharacterCreationService::ProjectHelgenStart(')
        self.assertIn('"STRE_QUEST_HelgenInvestigation"', projection)
        self.assertIn('quest->ScriptSetStage(10)', projection)
        for forbidden in ('ScriptSetStage(30)', 'ScriptSetStage(0)', 'EnsureQuestStarted', '->SetStage(', 'GetById(0x'):
            self.assertNotIn(forbidden, projection)
        fragment = ALT.split('Function Fragment_6()', 1)[1].split('EndFunction', 1)[0]
        self.assertIn('EnsurePostHelgenProjection()', fragment)
        helper = ALT.split('Bool Function EnsurePostHelgenProjection()', 1)[1]
        self.assertNotIn('Alias_', helper)
        self.assertNotIn('BeginCharacterCreation()', helper)
        self.assertNotIn('SetStage(20)', helper.split('MQ101.SetStage(20)')[0])
        self.assertIn('STREHelgenNPCCleanup.GetStageDone(40)', helper)
        self.assertIn('If PostHelgenProjectionStarted', helper)
        self.assertIn('If !STREHelgenNPCCleanup.Start()', helper)
        self.assertEqual(re.findall(r'MQ101.SetStage\((\d+)\)', ALT),
                         ['20','25','26','28','30','40','70','100','145','150','180','200','250','500','800','900'])
        self.assertEqual(ALT.count('BeginCharacterCreation()'), 3)

    def test_t0_after_projection_authorization_and_never_on_duplicate(self):
        begin = INV.split('Function BeginInvestigation()', 1)[1].split('EndFunction', 1)[0]
        self.assertLess(begin.index('If InvestigationState != 0'), begin.index('EnsurePostHelgenProjection'))
        self.assertEqual(begin.count('!SkyrimTogetherUtils.IsHelgenInvestigationStartAuthorized()'), 2)
        self.assertLess(begin.index('EnsurePostHelgenProjection'), begin.index('InvestigationStartGameTime ='))
        self.assertIn('If InvestigationState == 0', begin)
        self.assertIn('MultiplayerDeadlineArmed = MultiplayerCampaignObserved', begin)
        for name in ('ArmStandaloneBanditOccupationDeadline', 'EvaluateStandaloneBanditOccupationDeadline'):
            function = INV.split('Function '+name+'()', 1)[1].split('EndFunction', 1)[0]
            self.assertIn('MultiplayerCampaignObserved', function)
            self.assertIn('IsHelgenCampaignRequired()', function)
        self.assertIn('(InvestigationStartGameTime + 4.0)', INV)
        self.assertIn('AreAllRequiredPlayersOutsideHelgen()', INV)

    def test_duplicate_applied_and_individual_seating_precede_collective_barrier(self):
        applied = body(SERVER, 'void CharacterBuildService::OnCharacterBuildAppliedRequest(')
        self.assertLess(applied.index('InventoryHash'), applied.index('if (pBuild->Applied)'))
        self.assertLess(applied.index('SpellHash'), applied.index('if (pBuild->Applied)'))
        duplicate = body(applied, 'if (pBuild->Applied)')
        self.assertIn('return;', duplicate)
        self.assertNotIn('OnCharacterBuildApplied', duplicate)
        self.assertNotIn('SetLevel', duplicate)
        self.assertLess(applied.index('CharacterBuildNetworkState::Applied,\n        true'), applied.index('OnCharacterBuildApplied(*pPlayer)'))
        finalize = body(CREATION, 'void CharacterCreationService::FinalizeCompletedBuild(')
        self.assertIn('CreationSeating::FinalizeLocal', finalize)
        self.assertNotIn('IsHelgenInvestigationStartAuthorized', finalize)
        self.assertNotIn('Consume(', finalize)
        self.assertIn('CreationSeating::Tick', finalize)
        for path in ('Code/client/Services/Generic/CreationSeating.cpp', 'Code/common/CharacterCreation/CreationSeating.h'):
            self.assertNotIn('Helgen', read(path))

    def test_shared_authority_does_not_sync_quests_or_native_mq101(self):
        quest = read('Code/client/Services/Generic/QuestService.cpp')
        self.assertIn('STRE_QUEST_AlternateStart', quest)
        self.assertIn('STRE_QUEST_HelgenInvestigation', quest)
        campaign = read('Code/client/Services/Generic/CampaignService.cpp')
        self.assertIn('!wasAuthorized && acNotification.InvestigationStartAuthorized', campaign)
        self.assertIn('HelgenStartAuthorizedEvent', campaign)
        self.assertNotIn('ScriptSetStage', campaign)
        self.assertIn('m_helgenCampaignRequired.store(m_admissionState.GetAdmission().has_value())',
                      read('Code/client/Services/CampaignService.h'))
        for path in ('Code/server/Services/CampaignProtocolService.cpp', 'Code/server/Services/CharacterBuildService.cpp',
                     'Code/client/Services/Generic/CampaignService.cpp', 'Code/client/Services/Generic/CharacterCreationService.cpp'):
            self.assertNotRegex(read(path), r'MQ101\s*(?:\.|->)\s*(?:Script)?SetStage')
        for callback in ('OnDisconnected', 'OnMainMenuEntered', 'ResetForFreshCharacterCreation'):
            function = body(CREATION, 'CharacterCreationService::' + callback + '(')
            self.assertIn('m_helgenProjection.Reset()', function)
        server = read('Code/server/Services/CampaignProtocolService.cpp')
        proof = body(server, 'HelgenBuildEvidence CampaignProtocolService::ReadHelgenBuild(')
        for guard in ('m_world.valid', 'OwnerComponent', 'owner->GetOwner() != member', 'CampaignIdentity != acAdmission.AdmittedIdentity'):
            self.assertIn(guard, proof)

if __name__ == '__main__':
    unittest.main()
