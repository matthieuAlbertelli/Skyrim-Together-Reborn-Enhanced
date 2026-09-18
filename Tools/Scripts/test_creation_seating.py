"""Individual Applied seating boundaries; source checks, not native execution."""
import json
import re
import unittest
from test_remote_respawn_lab import read, body, ROOT, CREATION, LAB
SEATING = read("Code/client/Services/Generic/CreationSeating.cpp")

class CreationSeating(unittest.TestCase):
    def test_applied_is_individual_and_server_hash_guards_remain(self):
        receive = body(SEATING, "void Receive(")
        self.assertIn("CharacterBuildNetworkState::Applied", receive)
        self.assertNotIn("Present", SEATING)
        self.assertNotIn("AllApplied", SEATING)
        server = read("Code/server/Services/CharacterBuildService.cpp")
        applied = body(server, "void CharacterBuildService::OnCharacterBuildAppliedRequest(")
        for guard in ("pBuild->Revision != acPacket.Packet.Revision", "acPacket.Packet.InventoryHash !=", "acPacket.Packet.SpellHash !="):
            self.assertLess(applied.index(guard), applied.index("pBuild->Applied = true"))
        self.assertIn("GetAdmission(aPlayer)", server)
        self.assertIn("AdmittedIdentity->Player.Value", server)

    def test_mapping_matches_actual_manifest(self):
        table = body(read("Code/common/CharacterCreation/CreationSeating.h"), "inline std::optional<uint32_t> CreationSeatLocalFormId(")
        ids = [int(x, 16) for x in re.findall(r"0x[0-9A-F]+", table)]
        records = json.loads((ROOT / "docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json").read_text(encoding="utf-8"))["records"]
        lookup = {r["editorId"]: r for r in records}
        self.assertEqual(len(ids), 10)
        for i, form in enumerate(ids):
            self.assertEqual(form, int(lookup[f"STRE_FURN_PlayerSeat{i+1:02d}"]["expectedLocalFormId"], 16))
        self.assertIn("StandingCreationPositionIndex(players, intent.DurablePlayer)", SEATING)
        self.assertNotIn(".SlotId", SEATING)

    def test_remote_waits_for_committed_current_binding(self):
        self.assertIn("RemoteRespawnLab::CommittedActor", SEATING)
        query = body(LAB, "Actor* CommittedActor(")
        for guard in ("FinalBuildRevision != aRevision", "MaterializationState::Committed", "Bound(aWorld, job, job.Candidate)"):
            self.assertIn(guard, query)
        self.assertNotIn("Activate", query)
        self.assertIn('pending-actor', SEATING)

    def test_occupancy_checked_before_single_activation_without_teleport(self):
        self.assertEqual(SEATING.count("->Activate("), 1)
        self.assertLess(SEATING.index('papyrus.Get("ObjectReference", "IsFurnitureInUse")'), SEATING.index("->Activate("))
        self.assertLess(SEATING.index("intent.Projection.Issued = true"), SEATING.index("->Activate("))
        self.assertIn("(seat, false)", SEATING)  # include reservations
        self.assertIn("occupiedFurniture.handle.iBits", SEATING)
        for forbidden in ("MoveTo(", "SetPosition(", "SetStage(", "SetActorState", ".Send(", "SwitchRace", "Reset3D"):
            self.assertNotIn(forbidden, SEATING)

    def test_local_finalize_unlocks_before_seating_and_solo_has_no_network_dependency(self):
        finalize = body(CREATION, "void CharacterCreationService::FinalizeCompletedBuild(")
        self.assertLess(finalize.index("UnlockCharacterCreationControls()"), finalize.index("CreationSeating::FinalizeLocal"))
        self.assertLess(finalize.index("SetStopped()"), finalize.index("CreationSeating::FinalizeLocal"))
        self.assertLess(finalize.index("CreationSeating::FinalizeLocal"), finalize.index("ResetNetworkBuildState()"))
        self.assertIn("CreationSeating::Tick(m_world)", finalize)
        self.assertIn("solo.Local = solo.Solo = true", SEATING)
        self.assertIn("size_t index{}", SEATING)
        self.assertNotIn("kBuildSealSeconds", CREATION)

    def test_recovery_disconnect_and_duplicate_are_fenced(self):
        self.assertIn("pending-recovery-lock", SEATING)
        self.assertIn("rejected-identity-or-revision", SEATING)
        self.assertIn("rejected-duplicate-durable-binding", SEATING)
        self.assertIn("pending-entry-timeout-no-reactivation", SEATING)
        self.assertIn("CreationSeating::Clear()", body(CREATION, "void CharacterCreationService::OnDisconnected("))
        self.assertIn("CreationSeating::Clear()", body(CREATION, "bool CharacterCreationService::ResetForFreshCharacterCreation("))

    def test_wire_extension_is_bounded_and_transactional(self):
        source = read("Code/encoding/Messages/NotifyCharacterBuildState.cpp")
        self.assertIn("size > MaxSeatingIdentityBytes", source)
        self.assertIn("version != 1", source)
        self.assertLess(source.index("!aReader.ReadBytes"), source.index("SeatingCampaignId = std::move"))
