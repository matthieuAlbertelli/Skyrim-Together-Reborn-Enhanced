"""Source-contract checks for the creation-only extraction; never runs Skyrim.

Run from any directory with Python's standard library. These structural checks
complement TPTests; they do not certify native creation, lifetime or rendering.
"""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "Code/client/Services/Generic/CharacterService.cpp").read_text()


def body(source, marker):
    start = source.index(marker)
    opening = source.index("{\n", start)
    return source[opening + 2:source.index("\n}", opening)]


HELPER = body(SOURCE, "PrivateRemoteActor MaterializePrivateRemoteActor(")
CALLERS = [body(SOURCE, "void CharacterService::OnCharacterSpawn("),
           body(SOURCE, "Actor* CharacterService::CreateCharacterForEntity(")]


class PrivateRemoteMaterializerContract(unittest.TestCase):
    def check_caller(self, caller):
        self.assertEqual(caller.count("MaterializePrivateRemoteActor("), 1)
        gate = caller.index("if (acMessage.BaseId == GameId{} && acMessage.IsPlayer)")
        call = caller.index("MaterializePrivateRemoteActor(")
        existing = caller.index("else if (acMessage.BaseId != GameId{})")
        self.assertLess(caller.index("if (acMessage.FormId == GameId{})"), gate)
        self.assertLess(gate, call)
        self.assertLess(call, existing)
        private_branch = caller[gate:existing]
        for native in ("TESNPC::Create(", "FaceGenSystem::Setup(", "Actor::Create("):
            self.assertNotIn(native, private_branch)
        self.assertIn("acMessage.AppearanceBuffer, acMessage.ChangeFlags, acMessage.FaceTints", private_branch)
        self.assertIn("pNpc = materialized.pBase;", private_branch)
        self.assertIn("pActor = materialized.pActor;", private_branch)

    def test_t1_initial_spawn_uses_shared_helper(self):
        self.check_caller(CALLERS[0])

    def test_t2_deferred_spawn_uses_shared_helper(self):
        self.check_caller(CALLERS[1])

    def test_t3_native_order_and_borrowed_result(self):
        calls = re.findall(r"(?:TESNPC::Create|FaceGenSystem::Setup|Actor::Create)\([^;]+;", HELPER)
        self.assertEqual(calls, [
            "TESNPC::Create(acAppearanceBuffer, aChangeFlags);",
            "FaceGenSystem::Setup(aWorld, aEntity, acFaceTints);",
            "Actor::Create(pNpc, apLocation);"])
        self.assertIn("Actor* pActor = Actor::Create(pNpc, apLocation);", HELPER)
        self.assertIn("return {pNpc, pActor};", HELPER)
        self.assertIn("if (apCandidateTints)", HELPER)  # Slice 3 isolates tint storage only.
        self.assertNotIn("return nullptr", HELPER)
        self.assertEqual(re.findall(r'aTrace\("([^"]+)"\)', HELPER),
                         ["TESNPC-create", "FaceGen-setup", "Actor-create"])

    def test_t4_no_network_or_identity_in_materializer(self):
        for forbidden in ("Send(", "RemoteComponent", "LocalComponent", "PlayerComponent",
                          "ServerId", "PlayerId", "TakeOwnership", "Assignment",
                          "Respawn(", "Delete(", "Free(", "Rebind("):
            self.assertNotIn(forbidden, HELPER)
        signature = SOURCE[SOURCE.index("PrivateRemoteActor MaterializePrivateRemoteActor("):]
        signature = signature[:signature.index("{\n")]
        self.assertNotIn("CharacterSpawnRequest", signature)

    def test_t5_other_domains_stay_separate(self):
        for caller in CALLERS:
            self.assertIn("else if (acMessage.BaseId != GameId{})", caller)
            self.assertIn("pNpc->Deserialize(acMessage.AppearanceBuffer, acMessage.ChangeFlags);", caller)
            # The remaining native private-NPC block serves non-players only.
            self.assertEqual(caller.count("TESNPC::Create("), 1)
            self.assertEqual(caller.count("FaceGenSystem::Setup("), 1)
            self.assertEqual(caller.count("Actor::Create("), 1)
            self.assertIn("if (acMessage.BaseId != GameId{} || !acMessage.IsPlayer)", caller)
            bind = caller.index("if (pActor && STRE::CharacterCreation::IsPrivatePlayerCreation(")
            self.assertGreater(bind, caller.index("pActor = materialized.pActor;"))
            self.assertIn("probe->Rebind();", caller[bind:])
            self.assertIn("emplace_or_replace<RemotePlayerAppearanceBaseComponent>", caller[bind:])
        self.assertIn("GetGameId(acMessage.FormId)", CALLERS[0])

    def test_t6_empty_tints_still_delegate_to_unchanged_setup(self):
        facegen = (ROOT / "Code/client/Systems/FaceGenSystem.cpp").read_text()
        setup = body(facegen, "void FaceGenSystem::Setup(")
        self.assertRegex(setup, r"if \(acTints\.Entries\.empty\(\)\)\s+return;")
        self.assertLess(setup.index("return;"), setup.index("emplace_or_replace<FaceGenComponent>"))
        self.assertNotIn("remove<", setup)
        self.assertNotIn("empty()", HELPER)

    def test_t7_only_existing_spawn_entry_points(self):
        # Slice 3 adds one non-MASTER LAB adapter; ordinary spawn still has two callers.
        self.assertEqual(SOURCE.count("MaterializePrivateRemoteActor("), 4)
        lab = body(SOURCE, "Actor* STRE::RemoteRespawnLab::Materialize(")
        self.assertIn("#if (!IS_MASTER)", lab)
        self.assertIn("&aTints", lab)
        for path in (ROOT / "Code/client").rglob("*.cpp"):
            if path.name != "CharacterService.cpp":
                self.assertNotIn("MaterializePrivateRemoteActor(", path.read_text())


if __name__ == "__main__":
    unittest.main()
