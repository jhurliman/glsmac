"""Small real-LLVM fixtures proving that the enabled bug categories block CI."""

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("analyze_cpp", ROOT / "tools/analyze_cpp.py")
analysis = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(analysis)


@unittest.skipUnless(shutil.which("clang-tidy-15"), "real diagnostic tests require clang-tidy-15")
class CorrectnessChecks(unittest.TestCase):
    def analyze(self, source, header=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            (root / "src").mkdir()
            (root / ".clang-tidy").write_bytes((ROOT / ".clang-tidy").read_bytes())
            path = root / "src/probe.cpp"
            path.write_text(source + "\n", encoding="utf-8")
            if header is not None:
                (root / "src/probe.h").write_text(header + "\n", encoding="utf-8")
            database = [{"directory": str(root), "file": str(path),
                         "arguments": ["clang++-15", "-std=c++17", "-c", str(path)]}]
            (root / "compile_commands.json").write_text(json.dumps(database), encoding="utf-8")
            output = io.StringIO()
            with patch.object(analysis, "ROOT", root), contextlib.redirect_stdout(output):
                status = analysis.main(["--all", "--build-dir", str(root), "--jobs", "1"])
            return status, output.getvalue()

    def test_null_dereference_fails(self):
        status, output = self.analyze("int probe() { int* p = nullptr; return *p; }")
        self.assertEqual(status, 1, output)
        self.assertIn("clang-analyzer-core.NullDereference", output)

    def test_use_after_delete_fails(self):
        status, output = self.analyze("int probe() { auto* p = new int(1); delete p; return *p; }")
        self.assertEqual(status, 1, output)
        self.assertIn("clang-analyzer-cplusplus.NewDelete", output)

    def test_project_header_diagnostic_is_not_filtered_out(self):
        status, output = self.analyze('#include "probe.h"\nint probe() { return broken(); }',
                                      "inline int broken() { int* p = nullptr; return *p; }")
        self.assertEqual(status, 1, output)
        self.assertIn("probe.h:1:", output)
        self.assertIn("clang-analyzer-core.NullDereference", output)

    def test_double_free_fails(self):
        status, output = self.analyze("#include <cstdlib>\nvoid probe() { void* p = std::malloc(4); std::free(p); std::free(p); }")
        self.assertEqual(status, 1, output)
        self.assertIn("clang-analyzer-unix.Malloc", output)

    def test_valid_ownership_passes(self):
        status, output = self.analyze("#include <cstdlib>\nint probe() { auto* p = new int(1); int value = *p; delete p; void* q = std::malloc(4); std::free(q); return value; }")
        self.assertEqual(status, 0, output)


if __name__ == "__main__":
    unittest.main()
