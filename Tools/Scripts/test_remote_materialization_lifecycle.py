"""Slice 2A source fences, extended for the explicitly gated Slice 3 adapter."""

from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODEL = ROOT / "Code/common/CharacterCreation/RemoteMaterializationLifecycle.h"


class DormantLifecycleContract(unittest.TestCase):
    def test_t7_no_global_discovery_suppression(self):
        model = MODEL.read_text()
        self.assertNotRegex(model, r"\bstatic\b")
        self.assertEqual(re.findall(r"#include ([^\n]+)", model), ["<cstdint>"])
        discovery = (ROOT / "Code/client/Services/Generic/DiscoveryService.cpp").read_text()
        self.assertNotRegex(discovery, r"(?i)(suppress|disable|skip)Discovery")
        self.assertIn("m_dispatcher.trigger(ActorRemovedEvent(formId));", discovery)
        self.assertIn("m_dispatcher.update<ActorAddedEvent>();", discovery)

    def test_t8_only_gated_lab_uses_lifecycle_model(self):
        for directory in ("client", "server", "encoding"):
            for path in (ROOT / "Code" / directory).rglob("*"):
                if path.suffix in (".h", ".cpp"):
                    if path.name == "RemoteRespawnLab.cpp" and directory == "client":
                        source = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", path.read_text(), flags=re.S)
                        self.assertNotIn("RemoteMaterializationLifecycle Lifecycle", source)
                        continue
                    self.assertNotIn("RemoteMaterializationLifecycle", path.read_text(), str(path))
        # Keep the model dependency graph limited to tests, even through common headers.
        for path in (ROOT / "Code/common").rglob("*"):
            if path != MODEL and path.suffix in (".h", ".cpp"):
                self.assertNotIn("RemoteMaterializationLifecycle", path.read_text(), str(path))


if __name__ == "__main__":
    unittest.main()
