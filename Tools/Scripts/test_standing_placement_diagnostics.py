"""Standing placement's native boundary: source checks, never launches Skyrim."""
import re
import unittest
from test_remote_respawn_lab import read, body, CREATION

PLACEMENT = body(CREATION, 'bool CharacterCreationService::PlaceStandingForCreation(')
ADVANCE = body(CREATION, 'void CharacterCreationService::AdvanceCreationPlacement(')
SELECTION = read('Code/campaign_client/CampaignStandingPlacement.h')


class StandingPlacementDiagnostics(unittest.TestCase):
    def test_each_native_rejection_is_logged_and_success_follows_validation(self):
        # The only bare false is inside the diagnostic sink. All failed branches use it.
        self.assertEqual(PLACEMENT.count('return false;'), 1)
        self.assertIn('phase=standing-position-rejected reason={}', PLACEMENT)
        for reason in ('creation-position-index-out-of-range', 'player-missing', 'creation-quest-missing',
                       'creation-marker-missing', 'creation-marker-reference-mismatch', 'creation-marker-cell-missing', 'creation-marker-wrong-cell',
                       'player-cell-missing', 'player-marker-cell-mismatch',
                       'creation-marker-transform-invalid'):
            self.assertIn(f'return reject("{reason}")', PLACEMENT)
        self.assertIn('return reject(selection.Reason)', PLACEMENT)
        self.assertIn('reason=move-validation-failed detail={}', ADVANCE)
        self.assertLess(ADVANCE.index('ObserveCreationMove('), ADVANCE.index('phase=standing-position playerId='))

    def test_current_cell_is_used_not_save_parent_virtual(self):
        code = re.sub(r'//[^\n]*', '', PLACEMENT)
        self.assertNotIn('->GetParentCell()', code)
        self.assertIn('auto* cell = anchor->parentCell;', code)
        self.assertIn('player->parentCell != cell', code)
        self.assertIn('player->parentCell == cell', ADVANCE)
        self.assertIn('MoveTo(cell, position)', code)

    def test_entry_checks_position_without_posture_or_furniture_state(self):
        for forbidden in ('player-not-standing', 'sitSleep', 'GetSitState', 'WantToStand',
                          'actorState', 'ExtraDataType::Interaction', 'Activate(', 'ExitFurniture', 'SetActorState'):
            self.assertNotIn(forbidden, PLACEMENT + ADVANCE)
        self.assertIn('TESForm::GetById(player->formID) != player', PLACEMENT)
        self.assertIn('CreationPositionReached(actual, pending.Target)', ADVANCE)
        self.assertIn('ObserveCreationMove(sameCell, actual, pending.Target, elapsed)', ADVANCE)

    def test_durable_player_rank_is_the_only_multiplayer_index_source(self):
        self.assertIn('campaign.GetDurablePlayerIdForAuthentication()', PLACEMENT)
        self.assertIn('ResolveCampaignStandingPlacement(snapshot ? &*snapshot : nullptr, playerId)', PLACEMENT)
        self.assertIn('creationPositionIndex = *selection.Index;', PLACEMENT)
        self.assertIn('CreationMarkerLocalFormId(creationPositionIndex)', PLACEMENT)
        self.assertIn('ResolvePluginFormId("STRE_AlternateStart.esp", markerLocalId)', PLACEMENT)
        self.assertNotIn('GetAliasedRef', PLACEMENT)
        self.assertNotIn('StandingCreationPosition(', PLACEMENT)
        self.assertIn('const auto position = anchor->position;', PLACEMENT)
        self.assertIn('pending.TargetRotation = anchor->rotation;', PLACEMENT)
        self.assertIn('CreationMarkerTransformValid(position, anchor->rotation)', PLACEMENT)
        self.assertIn('cell->formID != expectedCellId', PLACEMENT)
        self.assertIn('player->SetRotation(pending.TargetRotation.x, pending.TargetRotation.y, pending.TargetRotation.z)', ADVANCE)
        for source in (PLACEMENT, SELECTION):
            for forbidden in ('GetAdmission()', 'CampaignSlotId', '.SlotId', '.IsValid()', 'GetLocalPlayerId()',
                              'TakeOwnership', '.Send(', '->Activate(', 'MoveTo(anchor', 'SetStage(', 'GetByEditorID'):
                self.assertNotIn(forbidden, source)
        self.assertIn('slot.PlayerId.data(), slot.PlayerId.size()', SELECTION)
        self.assertIn('ResolveStandingCreationPositionIndex(players, aLocalPlayerId)', SELECTION)
        self.assertIn('phase=standing-position playerId={} creationPositionIndex={} rosterCount={}', ADVANCE)

    def test_single_move_is_observed_without_sleep_or_retry_and_fenced_on_updates(self):
        self.assertEqual(PLACEMENT.count('->MoveTo('), 1)
        self.assertNotIn('MoveTo(', ADVANCE)
        self.assertNotIn('sleep', PLACEMENT + ADVANCE)
        self.assertLess(PLACEMENT.index('m_creationPlacement ='), PLACEMENT.index('->MoveTo('))
        update = body(CREATION, 'void CharacterCreationService::OnUpdate(')
        self.assertLess(update.index('AdvanceCreationPlacement();'), update.index('Recovering stage 20 flow'))
        authorize = body(CREATION, 'void CharacterCreationService::OnCampaignBootstrapAuthorized(')
        self.assertIn('m_creationPlacement ||', authorize)
        self.assertNotIn('OpenRaceMenu()', authorize)
        self.assertLess(ADVANCE.index('else if (complete)'), ADVANCE.index('OpenRaceMenu()'))
        for guard in ('player-identity-lost', 'target-cell-lost', 'anchor-identity-lost', 'quest-identity-lost',
                      'connection-changed', 'local-player-identity-changed', 'campaign-placement-changed',
                      'actual-position-nonfinite', 'position-timeout', 'cell-timeout'):
            self.assertIn(guard, ADVANCE)
        for marker in ('bool CharacterCreationService::ResetForFreshCharacterCreation(', 'void CharacterCreationService::Fail('):
            self.assertIn('m_creationPlacement.reset()', body(CREATION, marker))
        self.assertIn('AdvanceCreationPlacement("transport-disconnected")', body(CREATION, 'void CharacterCreationService::OnDisconnected('))

    def test_move_observation_logs_actual_values_and_uses_monotonic_deadline(self):
        for value in ('targetPosition=', 'actualPositionBefore=', 'actualPositionAfter=', 'distance=', 'targetRotation=',
                      'actualRotation=', 'targetCell=', 'actualCell=', 'elapsedMs=', 'sample=', 'actorValid='):
            self.assertIn(value, ADVANCE)
        self.assertIn('std::chrono::steady_clock::now()', ADVANCE)
        self.assertIn('std::chrono::milliseconds(250)', ADVANCE)

    def test_marker_table_matches_manifest_in_exact_index_order(self):
        import json
        from test_remote_respawn_lab import ROOT
        header = read('Code/common/CharacterCreation/StandingCreation.h')
        table = body(header, 'inline std::optional<uint32_t> CreationMarkerLocalFormId(')
        ids = [int(value, 16) for value in re.findall(r'0x[0-9A-F]+', table)]
        manifest = json.loads((ROOT / 'docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json').read_text(encoding='utf-8'))
        records = {r['editorId']: r for r in manifest['records']}
        self.assertEqual(len(ids), 10)
        for index, form_id in enumerate(ids):
            record = records[f'STRE_REFR_PlayerCreationMarker{index+1:02d}']
            self.assertEqual(record['signature'], 'REFR')
            self.assertEqual(int(record['expectedLocalFormId'], 16), form_id)

    def test_solo_keeps_index_zero_without_campaign_selection(self):
        self.assertIn('size_t creationPositionIndex = 0;', PLACEMENT)
        connected = body(PLACEMENT, 'if (connected)')
        self.assertIn('ResolveCampaignStandingPlacement', connected)
        self.assertEqual(PLACEMENT.count('ResolveCampaignStandingPlacement'), 1)
        self.assertIn('connected ? (snapshot ? snapshot->Roster.size() : 0) : 1', PLACEMENT)

    def test_production_admission_is_retained_on_start_and_snapshot(self):
        source = read('Code/client/Services/Generic/CampaignService.cpp')
        response = body(source, 'void CampaignService::OnCommandResponse(')
        start = response.split('else if (acResponse.Operation == CampaignProtocolOperation::Start)', 1)[1]
        start = start.split('else if (acResponse.Operation == CampaignProtocolOperation::Leave)', 1)[0]
        self.assertNotIn('Accept(', start)
        self.assertNotIn('Leave(', start)
        self.assertIn('m_admissionState.GetAdmission()', start)
        self.assertIn('m_admissionState.Accept(std::move(admission))', response)
        snapshot = body(source, 'bool CampaignService::ApplySnapshot(')
        self.assertIn('m_latestSnapshot = acSnapshot', snapshot)
        self.assertNotIn('m_admissionState.Accept(', snapshot)


if __name__ == '__main__':
    unittest.main()
