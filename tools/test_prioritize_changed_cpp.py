#!/usr/bin/env python3
"""Check actual Ninja target spelling and source-selection boundaries."""

import subprocess
import tempfile
import unittest
from pathlib import Path
from prioritize_changed_cpp import changed_sources, select_targets


class PriorityTests(unittest.TestCase):
    def test_ninja_graph_uses_platform_object_suffix_and_exact_source(self):
        graph = "\n".join([
            "CMakeFiles/GLSMAC.dir/src/test.cpp.obj: CXX_COMPILER__GLSMAC_Debug",
            "CMakeFiles/GLSMAC.dir/src/test.cpp.o: CXX_COMPILER__GLSMAC_Debug",
            "CMakeFiles/GLSMAC.dir/src/other.cpp.obj: CXX_COMPILER__GLSMAC_Debug",
            "CMakeFiles/vendor.dir/src/test.cpp.obj: CXX_COMPILER__vendor_Debug",
            "src/test.cpp: phony",
        ])
        self.assertEqual(select_targets(graph, {"src/test.cpp", "src/included.cpp"}), [
            "CMakeFiles/GLSMAC.dir/src/test.cpp.o",
            "CMakeFiles/GLSMAC.dir/src/test.cpp.obj",
        ])

    def test_git_comparison_skips_deletion_and_headers_without_replacing_full_build(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            def git(*args):
                return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()
            git("init", "-q")
            git("config", "user.name", "CI fixture")
            git("config", "user.email", "ci@example.invalid")
            (root / "src").mkdir()
            for name in ("changed.cpp", "deleted.cpp", "included.h"):
                (root / "src" / name).write_text("before\n")
            git("add", ".")
            git("commit", "-qm", "base")
            base = git("rev-parse", "HEAD")
            (root / "src/changed.cpp").write_text("after\n")
            (root / "src/included.h").write_text("after\n")
            (root / "src/deleted.cpp").unlink()
            (root / "src/new.cpp").write_text("new\n")
            git("add", ".")
            git("commit", "-qm", "change")
            self.assertEqual(changed_sources(root, base), {"src/changed.cpp", "src/new.cpp"})


if __name__ == "__main__":
    unittest.main()
