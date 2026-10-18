import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("affected_tests", Path(__file__).resolve().parents[1] / "affected_tests.py")
policy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy)


class AffectedTests(unittest.TestCase):
    def test_current_shared_base_contracts_select_existing_mock_consumers(self):
        for leaf, expected in (("top_buttons", "base_governor_settings"), ("queue_editor", "base_queue_editor")):
            path = "GLSMAC_data/default/ui/parts/game/popup/base_screen/" + leaf + ".gls.js"
            plan = policy.select([path])
            self.assertIn("GLSMAC_data/tests/" + expected + ".gls.js", plan["scripts"])
            self.assertIn("GLSMAC_data/tests/base_energy_display.gls.js", plan["scripts"])
            self.assertTrue(plan["native_all"])

    def test_snapshot_edit_keeps_independent_idle_restore_consumers(self):
        plan = policy.select(["src/game/backend/faction/SmacSimulationState.cpp"])
        self.assertTrue(plan["native_all"])
        self.assertIn("src/gse/tests/Backend.cpp", plan["native_sources"])
        self.assertIn("src/gse/tests/SmacSimulationState.cpp", plan["native_sources"])
        plan = policy.select(["src/game/backend/unit/Unit.cpp"], changed_lines="int version = 6; buffer.WriteInt(version);")
        self.assertTrue(plan["native_all"])

    def test_transitive_shared_fixture_and_deleted_module_are_not_omitted(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            files = {
                "GLSMAC_data/tests/view.gls.js": "#include('./_mock');",
                "GLSMAC_data/tests/_mock.gls.js": "#include('../default/ui/view');",
                "src/gse/tests/Consumer.cpp": '#include "fixtures/Shared.h"',
                "src/gse/tests/fixtures/Shared.h": '#include "Nested.h"',
                "src/gse/tests/fixtures/Nested.h": "",
            }
            for name, text in files.items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
            plan = policy.select(["GLSMAC_data/default/ui/view.gls.js"], root)
            self.assertEqual(plan["scripts"], ["GLSMAC_data/tests/view.gls.js"])
            plan = policy.select(["src/gse/tests/fixtures/Nested.h"], root)
            self.assertEqual(plan["native_sources"], ["src/gse/tests/Consumer.cpp"])
            self.assertIn("src/gse/tests/fixtures/Shared.h", plan["native_registration_sources"])
            self.assertFalse(plan["native_all"])
            (root / "src/gse/tests/fixtures/Nested.h").unlink()
            plan = policy.select(["src/gse/tests/fixtures/Nested.h"], root)
            self.assertEqual(plan["native_sources"], ["src/gse/tests/Consumer.cpp"])

    def test_shared_header_runtime_and_unknown_boundaries_broaden(self):
        for paths in (["src/game/backend/Game.h"], ["src/gc/Space.cpp"], ["src/game/backend/new-unmapped.cpp"], ["CMakeLists.txt"]):
            self.assertTrue(policy.select(paths)["native_all"], paths)
        plan = policy.select(["src/gse/Async.cpp"])
        self.assertIn("GLSMAC_data/tests/async.gls.js", plan["scripts"])
        self.assertIn("GLSMAC_data/tests/base_queue_editor.gls.js", plan["scripts"])

    def test_include_only_fixture_is_never_run_as_standalone(self):
        plan = policy.select(["GLSMAC_data/tests/_base_population_mocks.gls.js"])
        self.assertIn("GLSMAC_data/tests/base_population.gls.js", plan["scripts"])
        self.assertFalse(any(Path(path).name.startswith("_") for path in plan["scripts"]))


if __name__ == "__main__":
    unittest.main()
