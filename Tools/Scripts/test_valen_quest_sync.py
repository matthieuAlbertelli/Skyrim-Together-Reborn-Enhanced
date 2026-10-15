"""Valen quest eligibility and existing party-sync contracts; not runtime proof."""
import re
import unittest
from pathlib import Path

from test_remote_respawn_lab import body
from valen_helgen import Plugin, NARRATIVE, OWNER

ROOT = Path(__file__).resolve().parents[2]
CLIENT = (ROOT / 'Code/client/Services/Generic/QuestService.cpp').read_text()
SERVER = (ROOT / 'Code/server/Services/QuestService.cpp').read_text()
PLUGIN = Plugin((ROOT / 'GameFiles/Skyrim/STRE_AlternateStart.esp').read_bytes())


class ValenQuestSyncContracts(unittest.TestCase):
    def test_narrative_is_eligible_for_existing_quest_sync(self):
        exclusions = body(CLIENT, 'kNonSyncableQuestEditorIds =')
        self.assertNotIn('"' + NARRATIVE + '"', exclusions)
        quest_id = PLUGIN.ids[NARRATIVE]
        self.assertTrue(PLUGIN.values(quest_id, 'INDX'))
        vanilla_exclusions = body(CLIENT, 'kNonSyncableQuestIds =')
        ids = {int(value, 16) for value in re.findall(r'0x[0-9A-Fa-f]+', vanilla_exclusions)}
        self.assertNotIn(quest_id, ids)

    def test_dialogue_and_existing_technical_projections_stay_excluded(self):
        exclusions = body(CLIENT, 'kNonSyncableQuestEditorIds =')
        self.assertEqual(set(re.findall(r'"([^"]+)"', exclusions)), {
            'STRE_QUEST_AlternateStart', 'STRE_QUEST_HelgenInvestigation', OWNER,
        })

    def test_real_side_quest_does_not_need_experimental_misc_sync(self):
        quest_type = int.from_bytes(PLUGIN.one(PLUGIN.ids[NARRATIVE], 'DNAM')[8:], 'little')
        types = (ROOT / 'Code/client/Games/Skyrim/Forms/TESQuest.h').read_text()
        side_type = int(re.search(r'SideQuest\s*=\s*(\d+)', types)[1])
        self.assertEqual(quest_type, side_type)
        experimental_gate = re.search(r'if \(notify.ClientQuestType.*?\)', SERVER)[0]
        gated_types = {int(value) for value in re.findall(r'ClientQuestType == (\d+)', experimental_gate)}
        self.assertEqual(gated_types, {0, 6})
        self.assertNotIn(quest_type, gated_types)
        self.assertIn('if (!bEnableMiscQuestSync)', body(SERVER, experimental_gate))

    def test_start_stage_stop_use_existing_updates_for_any_party_member(self):
        for event in ('TESQuestStartStopEvent', 'TESQuestStageEvent'):
            handler = body(CLIENT, 'QuestService::OnEvent(const ' + event)
            self.assertIn('IsNonSyncableQuest(pQuest)', handler)
            self.assertIn('IsInParty()', handler)
            self.assertIn('ScopedQuestOverride::IsOverriden()', handler)
            self.assertIn('RequestQuestUpdate update;', handler)
            self.assertIn('m_world.GetTransport().Send(update)', handler)
            self.assertNotIn('IsLeader', handler)
        self.assertIn('stopped ? RequestQuestUpdate::Stopped : RequestQuestUpdate::Started', CLIENT)
        self.assertIn('update.Status = RequestQuestUpdate::StageUpdate', CLIENT)
        relay = body(SERVER, 'void QuestService::OnQuestChanges(')
        self.assertIn('partyComponent.JoinedPartyId.has_value()', relay)
        self.assertIn('SendToParty(notify, partyComponent, acMessage.GetSender())', relay)
        self.assertNotIn('IsLeader', relay)

    def test_remote_stage_runs_existing_stage_and_objective_path(self):
        receiver = body(CLIENT, 'void QuestService::OnQuestUpdate(')
        for status in ('Started', 'StageUpdate'):
            clause = receiver.split('case NotifyQuestUpdate::' + status + ':', 1)[1].split('break;', 1)[0]
            self.assertIn('pQuest->ScriptSetStage(aUpdate.Stage)', clause)
        stopped = receiver.split('case NotifyQuestUpdate::Stopped:', 1)[1].split('break;', 1)[0]
        self.assertIn('StopQuest(formId)', stopped)
        qf = (ROOT / 'GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_ValenHelgen_030D6B14.psc').read_text()
        self.assertIn('SetObjectiveDisplayed(10, True)', qf)
        self.assertIn('SetActive(True)', qf)


if __name__ == '__main__':
    unittest.main()
