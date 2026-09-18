"""Slice 2A source fences, extended for the official final creation adapter."""

from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODEL = ROOT / "Code/common/CharacterCreation/RemoteMaterializationLifecycle.h"


class FinalCreationLifecycleContract(unittest.TestCase):
    def test_t7_no_global_discovery_suppression(self):
        model = MODEL.read_text()
        self.assertNotRegex(model, r"\bstatic\b")
        self.assertEqual(re.findall(r"#include ([^\n]+)", model), ["<cstdint>"])
        discovery = (ROOT / "Code/client/Services/Generic/DiscoveryService.cpp").read_text()
        self.assertNotRegex(discovery, r"(?i)(suppress|disable|skip)Discovery")
        self.assertIn("m_dispatcher.trigger(ActorRemovedEvent(formId));", discovery)
        self.assertIn("m_dispatcher.update<ActorAddedEvent>();", discovery)

    def test_t8_only_official_final_adapter_uses_lifecycle_model_in_all_builds(self):
        for directory in ("client", "server", "encoding"):
            for path in (ROOT / "Code" / directory).rglob("*"):
                if path.suffix in (".h", ".cpp"):
                    if path.name == "RemoteRespawnLab.cpp" and directory == "client":
                        source = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", path.read_text(), flags=re.S)
                        self.assertIn("RemoteMaterializationLifecycle Lifecycle", source)
                        self.assertIn("aJob.Lifecycle.Reserve(", source)
                        self.assertIn("aJob.Lifecycle.Commit(", source)
                        continue
                    self.assertNotIn("RemoteMaterializationLifecycle", path.read_text(), str(path))
        # No other production path may acquire the model through common headers.
        for path in (ROOT / "Code/common").rglob("*"):
            if path != MODEL and path.suffix in (".h", ".cpp"):
                self.assertNotIn("RemoteMaterializationLifecycle", path.read_text(), str(path))


if __name__ == "__main__":
    unittest.main()
