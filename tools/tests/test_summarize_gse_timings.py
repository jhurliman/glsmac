import importlib.util
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location("timings", Path(__file__).resolve().parents[1] / "summarize_gse_timings.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class TimingTests(unittest.TestCase):
    def test_completed_body_and_teardown_groups_parse_and_sort(self):
        result = module.summarize("  group started...\n[GSE timing] 20.125 ms | src/gse/tests/One.cpp | first\n[GSE timing] 103.500 ms | C:\\src\\Two.cpp | second\n  hung group...\n")
        self.assertEqual(result["measured_group_count"], 2)
        self.assertEqual(result["groups"][0]["name"], "second")
        self.assertEqual(result["groups"][0]["source"], "C:/src/Two.cpp")
        self.assertAlmostEqual(result["summed_group_seconds"], .123625)
