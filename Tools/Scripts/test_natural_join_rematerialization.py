"""Corrective Slice 3 contracts; no game launch or native rendering claim."""
import math
import re
import struct
import unittest

from audit_mq101_quickstart5 import parse_records, record_payload, iter_subrecords
from test_remote_respawn_lab import ROOT, read, body, LAB, SERVICE, CREATION, ACTOR

PROJECTION = read("Code/client/Services/Generic/RemoteActorProjection.cpp")
POLICY = read("Code/common/CharacterCreation/FinalRespawn.h")
PSC_PATH = ROOT / "GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc"
PSC = PSC_PATH.read_text(encoding="cp1252")


class NaturalJoinRematerialization(unittest.TestCase):
    def test_t5_no_race_domain_or_family_gate_in_lab_or_shared_projection(self):
        for source in (LAB, PROJECTION, POLICY):
            code = re.sub(r"//[^\n]*|/\*.*?\*/", "", source, flags=re.S)
            for forbidden in (r"AppearanceDomain", r"IsSupported.*Race", r"allowlist", r"beast", r"humanoid",
                              r"Khajiit", r"Argonian", r"\b(?:Nord|Orc|Elf|Human)(?:Race)?\b", r"0x0*1374[0-9]"):
                self.assertNotRegex(code, re.compile(forbidden, re.I))
        self.assertNotRegex(body(LAB, "bool Geometry("), r"(?:headpartsCount\s*==\s*0|!base->headpartsCount)")

    def test_t6_descriptor_only_validates_materialized_result(self):
        for forbidden in ("ClassifyAppearance", "SameRace", "raceChanged", "SexChange", "SetRace(", "SetSex("):
            self.assertNotIn(forbidden, LAB)
        adapter = body(SERVICE, "Actor* STRE::RemoteRespawnLab::Materialize(")
        self.assertNotIn("Descriptor", adapter)
        geometry = body(LAB, "bool Geometry(")
        self.assertIn("base->raceForm.race != target", geometry)
        self.assertIn("base->actorData.IsFemale()", geometry)

    def test_t8_native_setup_is_shared_by_both_natural_paths_and_lab(self):
        for marker in ("void CharacterService::OnCharacterSpawn(", "Actor* CharacterService::CreateCharacterForEntity("):
            caller = body(SERVICE, marker)
            self.assertEqual(caller.count("RemoteActorProjection::Initialize("), 1)
            self.assertIn("nullptr, &location", caller)
            self.assertNotIn("SetActorValues(", caller)
            self.assertNotIn("SetIgnoreFriendlyHit(", caller)
        self.assertIn("RemoteActorProjection::Initialize(candidate, true, location, aJob.Values)", LAB)
        initialize = body(PROJECTION, "void Initialize(")
        for call in ("SetRemote(true)", "Place(", "SetActorValues(", "SetPlayer(aPlayer)", "SetIgnoreFriendlyHit(true)", "SetPlayerRespawnMode()"):
            self.assertIn(call, initialize)

    def test_t8_waiting_for_3d_and_state_projection_are_shared(self):
        updates = body(SERVICE, "void CharacterService::RunRemoteUpdates(")
        self.assertIn("RemoteActorProjection::ReadyFor3D(pActor)", updates)
        self.assertIn("RemoteActorProjection::ReadyFor3D(aCandidate)", LAB)
        self.assertIn("RemoteActorProjection::Complete3D(pActor", updates)
        self.assertIn("RemoteActorProjection::Complete3D(candidate", LAB)
        self.assertIn("aActor && aActor->GetNiNode()", body(PROJECTION, "bool ReadyFor3D("))
        complete = body(PROJECTION, "void Complete3D(")
        for call in ("SetActorInventory(", "SetFactions(", "LoadAnimationVariables("):
            self.assertIn(call, complete)
        # Keep the existing wire action/diff baseline on the same logical entity.
        self.assertNotIn("AnimationSystem::Setup", LAB)
        self.assertNotIn("InterpolationSystem::Setup", LAB)
        self.assertIn("interpolation->TimePoints.back().Variables", LAB)

    def test_initial_spawn_receives_remote_location_before_native_exposure(self):
        create = body(ACTOR, "GamePtr<Actor> Actor::Create(")
        spawn = create.index("ModManager::Get()->Spawn")
        for member in ("Cell", "WorldSpace", "Position", "Rotation"):
            self.assertLess(create.index("apLocation->" + member), spawn)
        self.assertIn("Spawn(position, rotation, pCell, pWorldSpace, pActor)", create)
        self.assertIn("Actor::Create(pNpc, apLocation)", SERVICE)
        advance = body(LAB, "void Advance(")
        self.assertLess(advance.index("ActorSpawnLocation location{cell, space, aJob.Position, aJob.Rotation}"), advance.index("Materialize("))
        self.assertIn("aJob.Tints, location)", advance)
        self.assertNotIn("PlayerCharacter::Get", LAB)
        capture = body(LAB, "FinalRespawnAdmissionDecision Capture(")
        self.assertIn("aJob.Position = actor->position", capture)
        self.assertIn("aJob.Rotation = actor->rotation", capture)

    def test_t7_t9_projection_has_no_logical_identity_or_network_lifecycle(self):
        for source in (LAB, PROJECTION):
            for forbidden in ("RequestServerAssignment", "TakeOwnership", ".Send(", "RequestRespawn", "CharacterSpawnRequest"):
                self.assertNotIn(forbidden, source)
        for forbidden in ("RemoteComponent", "PlayerComponent", "LocalComponent", "emplace", "World::"):
            self.assertNotIn(forbidden, PROJECTION)

    def test_t11_posture_and_furniture_are_not_eligibility_or_completion_requirements(self):
        self.assertNotIn("seat-restore", LAB)
        for source in (LAB, POLICY):
            code = re.sub(r"//[^\n]*|/\*.*?\*/", "", source, flags=re.S)
            for forbidden in ("sitSleep", "GetSitState", "WantToStand", "Activate(",
                              "ExtraDataType::Interaction", "ExitFurniture"):
                self.assertNotIn(forbidden, code)
            self.assertNotRegex(code, r"(?:actorState\.(?:flags1|flags2)|aFlags[12])\s*(?:[|&^]?=(?!=)|\+\+|--)")
        safe = body(LAB, "bool SafeActor(")
        for retained in ("IsRemotePlayer()", "IsDead()", "IsDisabled()", "IsInCombat()", "IsMount()", "FinalRespawnActorStateSafe"):
            self.assertIn(retained, safe)

    def test_both_ck_entry_fragments_share_posture_independent_bootstrap(self):
        for number in (0, 4):
            fragment = PSC.split(f"Function Fragment_{number}()", 1)[1].split("EndFunction", 1)[0]
            self.assertIn("BeginCharacterCreation()", fragment)
            self.assertNotIn("MoveTo", fragment)
        standing = PSC.split("Function BeginCharacterCreation()", 1)[1].split("EndFunction", 1)[0]
        self.assertIn('Game.GetFormFromFile(0x0001B771, "STRE_AlternateStart.esp")', standing)
        self.assertIn("playerRef.MoveTo(startRef)", standing)
        self.assertLess(standing.index("playerRef.GetParentCell() != startCell"), standing.index("SetStage(20)"))
        self.assertIn('playerRef.GetDistance(startRef) > 32.0', standing)
        self.assertNotRegex(PSC, r"GetSitState|WantToStand|sitSleep|\.Activate\(|ExitFurniture")
        self.assertNotRegex(PSC, r"MoveTo\([^\n]*Seat")
        # The shipped script is rebuilt too, not just a source-only modification.
        pex = (ROOT / "GameFiles/Skyrim/scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex").read_bytes()
        self.assertIn(b"BeginCharacterCreation", pex)
        self.assertIn(b"Creation placement ready; entering stage 20", pex)
        self.assertNotIn(b"GetSitState", pex)

    def test_placement_uses_sealed_player_rank_before_racemenu(self):
        authorize = body(CREATION, "void CharacterCreationService::OnCampaignBootstrapAuthorized(")
        self.assertLess(authorize.index("PlaceStandingForCreation()"), authorize.index("AdvanceCreationPlacement()"))
        advance = body(CREATION, "void CharacterCreationService::AdvanceCreationPlacement(")
        self.assertLess(advance.index("else if (complete)"), advance.index("OpenRaceMenu()"))
        placement = body(CREATION, "bool CharacterCreationService::PlaceStandingForCreation(")
        for evidence in ("RosterSealed", "GetDurablePlayerIdForAuthentication()", "playerId", "ResolveCampaignStandingPlacement", "CreationMarkerLocalFormId(creationPositionIndex)", "MoveTo(cell, position)"):
            self.assertIn(evidence, placement)
        for forbidden in ("GetLocalPlayerId()", ".Activate(", "->Activate(", ".Send(", "SetStage("):
            self.assertNotIn(forbidden, placement)

    def test_actual_ck_marker_and_ten_distinct_anchors(self):
        data = (ROOT / "GameFiles/Skyrim/STRE_AlternateStart.esp").read_bytes()
        records = {r.form_id: r for r in parse_records(data)}
        def subs(form_id):
            return list(iter_subrecords(record_payload(data, records[form_id])))
        marker = dict(subs(0x0301B771))
        self.assertEqual(struct.unpack("<I", marker["NAME"])[0], 0x34)  # XMarkerHeading, not furniture.
        local_ids = [0xD6B08, 0xD6B09, 0xD6B13, 0xD6B12, 0xD6B0A,
                     0xD6B11, 0xD6B0B, 0xD6B10, 0xD6B0D, 0xD6B0F]
        positions = []
        for index, local_id in enumerate(local_ids):
            anchor = next(r.form_id for r in records.values() if r.form_id & 0xFFFFFF == local_id)
            record = dict(subs(anchor))
            self.assertEqual(records[anchor].signature, "REFR")
            self.assertEqual(record["EDID"].rstrip(b"\0").decode(), f"STRE_REFR_PlayerCreationMarker{index+1:02d}")
            self.assertEqual(struct.unpack("<I", record["NAME"])[0], 0x34)
            transform = struct.unpack("<6f", record["DATA"])
            self.assertTrue(all(math.isfinite(v) for v in transform))
            positions.append(transform[:3])
        # ADR-0025 reuses these as manually authored chair approaches. Distinct
        # positions remain required; no arbitrary spacing proves entry clearance.
        for i, first in enumerate(positions):
            for second in positions[:i]:
                self.assertNotEqual(first, second)


if __name__ == "__main__":
    unittest.main()
