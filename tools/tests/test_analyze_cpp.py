"""Regression coverage for scope selection and analyzer failures (no LLVM needed)."""

import importlib.util
import contextlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location("analyze_cpp", Path(__file__).resolve().parents[1] / "analyze_cpp.py")
analysis = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(analysis)


class AnalysisTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name).resolve()
        self.sources = ["src/first.cpp", "src/nested/second.cpp"]
        for source in self.sources:
            self.touch(source)

    def touch(self, path):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text("", encoding="utf-8")

    def test_changed_sources_are_whole_translation_units(self):
        selected, full = analysis.select_sources(self.root, self.sources,
                                                [self.sources[0], "README.md", "src/gse/tests/GSE.cpp"])
        self.assertEqual(selected, [self.sources[0]])
        self.assertFalse(full)

    def test_headers_and_build_policy_widen_to_all_sources_even_when_deleted(self):
        for path in ["src/deleted.h", "src/template.inl", "CMakeLists.txt", "cmake/options.cmake",
                     ".clang-tidy", "tools/analyze_cpp.py", ".github/workflows/cpp-analysis.yml"]:
            with self.subTest(path=path):
                self.assertEqual(analysis.select_sources(self.root, self.sources, [path]), (self.sources, True))

    def test_new_source_missing_from_database_fails_instead_of_skipping(self):
        self.touch("src/new.cpp")
        for full in [False, True]:
            with self.assertRaisesRegex(ValueError, "Reconfigure CMake"):
                analysis.select_sources(self.root, self.sources, ["src/new.cpp"], full)

    def test_only_the_known_windows_implementation_is_excluded(self):
        path = "src/network/impl/Windows.cpp"
        self.touch(path)
        self.assertEqual(analysis.select_sources(self.root, self.sources, [path]), ([], False))
        unknown = "src/network/impl/OtherWindows.cpp"
        self.touch(unknown)
        with self.assertRaisesRegex(ValueError, "Reconfigure CMake"):
            analysis.select_sources(self.root, self.sources, [unknown])

    def test_included_colormap_changes_select_the_including_translation_unit(self):
        fragment = "src/ui/dom/Object.colormap.cpp"
        owner = "src/ui/dom/Object.cpp"
        self.touch(fragment)
        self.touch(owner)
        sources = sorted(self.sources + [owner])
        self.assertEqual(analysis.select_sources(self.root, sources, [fragment]), (sources, True))
        (self.root / fragment).unlink()
        self.assertEqual(analysis.select_sources(self.root, sources, [fragment]), (sources, True))

    def test_cli_reports_the_windows_exclusion_without_analyzing_it(self):
        path = "src/network/impl/Windows.cpp"
        self.touch(path)
        database = [{"directory": str(self.root), "file": source} for source in self.sources]
        (self.root / "compile_commands.json").write_text(json.dumps(database), encoding="utf-8")
        output = io.StringIO()
        with patch.object(analysis, "ROOT", self.root), \
                patch.object(analysis, "changed_paths", return_value=[path]), \
                patch.object(analysis.shutil, "which", return_value="clang-tidy-15"), \
                patch.object(analysis.subprocess, "check_output", return_value="LLVM version 15.0.7"), \
                patch.object(analysis, "run_analysis") as run, \
                contextlib.redirect_stdout(output):
            status = analysis.main(["--build-dir", str(self.root)])
        self.assertEqual(status, 0)
        run.assert_not_called()
        self.assertIn(f"Skipping {path}: Windows-only", output.getvalue())
        summary = json.loads((self.root / "analysis-results/summary.json").read_text())
        self.assertEqual(summary["results"], [])
        self.assertEqual(summary["skipped_sources"], {path: analysis.LINUX_EXCLUDED_SOURCES[path]})

    def test_deleted_source_and_nonproduction_files_are_skipped(self):
        selected, full = analysis.select_sources(self.root, self.sources,
                                                ["src/deleted.cpp", "src/tmp/embed.cpp", "tools/embedder.cpp"])
        self.assertEqual(selected, [])
        self.assertFalse(full)

    def test_database_paths_are_resolved_deduplicated_and_scoped(self):
        database = [{"directory": str(self.root), "file": path} for path in
                    [self.sources[0], str(self.root / self.sources[0]), self.sources[1],
                     "src/gse/tests/GSE.cpp", "src/tmp/embed.cpp", "tools/embedder.cpp"]]
        (self.root / "compile_commands.json").write_text(json.dumps(database), encoding="utf-8")
        self.assertEqual(analysis.compilation_sources(self.root, self.root), self.sources)

    def test_explicit_sources_are_normalized_deduplicated_complete_units(self):
        requested = [self.sources[1], "./src/nested/../first.cpp", str(self.root / self.sources[0])]
        self.assertEqual(analysis.select_explicit_sources(self.root, self.sources, requested), self.sources)

    def test_explicit_sources_reject_invalid_scope_instead_of_skipping(self):
        for path in ["src/header.h", "src/tmp/embed.cpp", "src/gse/tests/GSE.cpp", "tools/embedder.cpp",
                     "src/ui/dom/Object.colormap.cpp", "src/network/impl/Windows.cpp"]:
            self.touch(path)
            with self.subTest(path=path), self.assertRaises(ValueError):
                analysis.select_explicit_sources(self.root, self.sources, [self.sources[0], path])
        with self.assertRaisesRegex(ValueError, "outside"):
            analysis.select_explicit_sources(self.root, self.sources, ["../outside.cpp"])
        with self.assertRaisesRegex(ValueError, "missing"):
            analysis.select_explicit_sources(self.root, self.sources, ["src/missing.cpp"])
        self.touch("src/new.cpp")
        with self.assertRaisesRegex(ValueError, "Reconfigure CMake"):
            analysis.select_explicit_sources(self.root, self.sources, ["src/new.cpp"])

    def test_explicit_source_symlink_cannot_escape_checkout(self):
        with tempfile.TemporaryDirectory() as outside:
            target = Path(outside) / "outside.cpp"
            target.write_text("", encoding="utf-8")
            link = self.root / "src/escape.cpp"
            try:
                link.symlink_to(target)
            except (OSError, NotImplementedError):
                self.skipTest("file symlinks unavailable")
            with self.assertRaisesRegex(ValueError, "outside"):
                analysis.select_explicit_sources(self.root, self.sources, ["src/escape.cpp"])

    def test_explicit_cli_never_queries_git_or_widens_header_scope(self):
        database = [{"directory": str(self.root), "file": source} for source in self.sources]
        (self.root / "compile_commands.json").write_text(json.dumps(database), encoding="utf-8")
        self.touch("src/changed.h")
        with patch.object(analysis, "ROOT", self.root), \
                patch.object(analysis, "changed_paths", side_effect=AssertionError("Git scope requested")) as changed, \
                patch.object(analysis, "git_output", side_effect=AssertionError("Git command requested")) as git, \
                patch.object(analysis, "select_sources", side_effect=AssertionError("automatic scope requested")) as automatic, \
                patch.object(analysis.shutil, "which", return_value="clang-tidy-15"), \
                patch.object(analysis.subprocess, "check_output", return_value="LLVM version 15.0.7"), \
                patch.object(analysis.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "", "")) as run, \
                contextlib.redirect_stdout(io.StringIO()):
            status = analysis.main(["--source", "./src/first.cpp", "--source", str(self.root / self.sources[0]),
                                    "--build-dir", str(self.root)])
        self.assertEqual(status, 0)
        changed.assert_not_called()
        git.assert_not_called()
        automatic.assert_not_called()
        run.assert_called_once()
        self.assertEqual(run.call_args.args[0][-1], str(self.root / self.sources[0]))
        summary = json.loads((self.root / "analysis-results/summary.json").read_text())
        self.assertEqual(summary["scope"], "explicit")
        self.assertEqual(summary["requested_sources"], [self.sources[0]])
        self.assertEqual(summary["changed_paths"], [])
        self.assertEqual(summary["skipped_sources"], {})

    def test_explicit_cli_rejects_scope_combinations_and_missing_argument(self):
        for arguments in [["--source", self.sources[0], "--all"],
                          ["--base", "HEAD", "--source", self.sources[0]], ["--source"]]:
            with self.subTest(arguments=arguments), contextlib.redirect_stderr(io.StringIO()), \
                    self.assertRaises(SystemExit) as result:
                analysis.main(arguments)
            self.assertEqual(result.exception.code, 2)

    def test_explicit_cli_rejects_invalid_request_before_running_analyzer(self):
        database = [{"directory": str(self.root), "file": source} for source in self.sources]
        (self.root / "compile_commands.json").write_text(json.dumps(database), encoding="utf-8")
        with patch.object(analysis, "ROOT", self.root), \
                patch.object(analysis, "changed_paths", side_effect=AssertionError("Git scope requested")), \
                patch.object(analysis, "run_analysis") as run, contextlib.redirect_stderr(io.StringIO()):
            status = analysis.main(["--source", self.sources[0], "--source", "../outside.cpp",
                                    "--build-dir", str(self.root)])
        self.assertEqual(status, 2)
        run.assert_not_called()

    def test_git_selection_includes_commits_index_worktree_and_untracked(self):
        def git(*args):
            return subprocess.check_output(["git", "-C", str(self.root), *args], stderr=subprocess.STDOUT)
        git("init", "-q")
        git("add", ".")
        git("-c", "user.name=Test", "-c", "user.email=test@example.invalid", "commit", "-qm", "base")
        base = git("rev-parse", "HEAD").decode().strip()
        self.touch("src/committed.cpp")
        git("add", ".")
        git("-c", "user.name=Test", "-c", "user.email=test@example.invalid", "commit", "-qm", "change")
        self.touch("src/staged.cpp")
        git("add", ".")
        (self.root / self.sources[0]).write_text("changed", encoding="utf-8")
        self.touch("src/untracked file.cpp")
        self.assertEqual(analysis.changed_paths(self.root, base),
                         ["src/committed.cpp", "src/first.cpp", "src/staged.cpp", "src/untracked file.cpp"])

    def test_diagnostics_and_analyzer_failure_are_preserved(self):
        result = subprocess.CompletedProcess([], 1, "source:1: error: bug\n", "compiler error\n")
        with patch.object(analysis.subprocess, "run", return_value=result):
            report, output = analysis.run_analysis(self.root, self.root, "clang-tidy", self.sources[0], 1, self.root / "logs")
        self.assertEqual(report["exit_code"], 1)
        self.assertEqual(Path(report["log"]).read_text(), output)
        self.assertIn("source:1: error: bug", output)
        self.assertIn("compiler error", output)

    def test_timeout_fails_and_keeps_partial_diagnostics(self):
        with patch.object(analysis.subprocess, "run", side_effect=subprocess.TimeoutExpired([], 1, b"partial diagnostic")):
            report, output = analysis.run_analysis(self.root, self.root, "clang-tidy", self.sources[0], 1, self.root / "logs")
        self.assertEqual(report["exit_code"], 124)
        self.assertIn("partial diagnostic", output)

    def test_cli_returns_failure_when_any_translation_unit_fails(self):
        database = [{"directory": str(self.root), "file": source} for source in self.sources]
        (self.root / "compile_commands.json").write_text(json.dumps(database), encoding="utf-8")
        for scope in [["--all"], ["--source", self.sources[0], "--source", self.sources[1]]]:
            with self.subTest(scope=scope), patch.object(analysis, "ROOT", self.root), \
                    patch.object(analysis.shutil, "which", return_value="clang-tidy-15"), \
                    patch.object(analysis.subprocess, "check_output", return_value="LLVM version 15.0.7"), \
                    patch.object(analysis.subprocess, "run", side_effect=[
                        subprocess.CompletedProcess([], 0, "", ""),
                        subprocess.CompletedProcess([], 1, "error: correctness finding", "")]), \
                    contextlib.redirect_stdout(io.StringIO()):
                status = analysis.main(scope + ["--build-dir", str(self.root), "--jobs", "1"])
            self.assertEqual(status, 1)
            summary = json.loads((self.root / "analysis-results/summary.json").read_text())
            self.assertEqual(summary["failures"], 1)
            self.assertEqual(len(summary["results"]), 2)
            self.assertEqual(summary["scope"], "full" if scope == ["--all"] else "explicit")
            self.assertEqual(summary.get("requested_sources"), None if scope == ["--all"] else self.sources)


if __name__ == "__main__":
    unittest.main()
