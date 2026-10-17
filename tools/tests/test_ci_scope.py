import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("ci_scope", Path(__file__).resolve().parents[1] / "ci_scope.py")
policy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy)


class ScopeTests(unittest.TestCase):
    def test_draft_gameplay_and_source_directory_cmake_use_linux(self):
        for path in ("src/game/backend/base/Rule.cpp", "src/game/backend/base/Rule.h",
                     "src/game/backend/base/CMakeLists.txt", "src/gse/tests/CMakeLists.txt",
                     "GLSMAC_data/default/game/rules.gls.js"):
            self.assertEqual(policy.select_scope("pull_request", {"pull_request": {"draft": True}}, [path])[:2], (True, False), path)

    def test_platform_paths_expand_drafts(self):
        for path in ("CMakeLists.txt", "CMakePresets.json", "tools/check.cmake",
                     "src/network/impl/Windows.cpp", "src/env/Env.h", "src/util/UUID.cpp",
                     "src/gc/Space.cpp", "src/common/MTModule.h", "src/graphics/opengl/OpenGL.cpp",
                     ".github/workflows/regression.yml", "tools/ci_scope.py", "dependencies/SDL2/CMakeLists.txt",
                     "resource/icon/glsmac.ico", "packaging/Info.plist", "cmake/toolchain.txt", "vcpkg.json", "deps/conan.lock"):
            self.assertEqual(policy.select_scope("pull_request", {"pull_request": {"draft": True}}, [path])[:2], (True, True), path)

    def test_ready_main_and_queue_do_not_repeat_platform_matrix(self):
        paths = ["src/game/backend/base/Rule.cpp"]
        self.assertEqual(policy.select_scope("pull_request", {"pull_request": {"draft": False}}, paths)[:2], (True, False))
        self.assertEqual(policy.select_scope("push", {"ref": "refs/heads/main"}, paths)[:2], (True, False))
        self.assertEqual(policy.select_scope("merge_group", {}, paths)[:2], (True, False))
        self.assertEqual(policy.select_scope("merge_group", {}, ["src/env/Win32.h"])[:2], (True, True))

    def test_manual_release_and_explicit_checkpoint_always_full(self):
        self.assertEqual(policy.select_scope("workflow_dispatch", {}, ["docs/evidence.md"])[:2], (True, True))
        self.assertEqual(policy.select_scope("push", {"ref": "refs/heads/stable"}, [])[:2], (True, True))
        for draft in (False, True):
            event = {"pull_request": {"draft": draft, "labels": [{"name": "ci:full"}]}}
            self.assertEqual(policy.select_scope("pull_request", event, ["docs/evidence.md"])[:2], (True, True))
            event["pull_request"]["labels"] = [{"name": "unrelated"}]
            self.assertEqual(policy.select_scope("pull_request", event, ["docs/evidence.md"])[:2], (False, False))

    def test_markdown_only_skips_engine_jobs_but_mixed_change_does_not(self):
        for event_name, event in (("pull_request", {"pull_request": {"draft": True}}),
                                  ("push", {"ref": "refs/heads/main"}), ("merge_group", {})):
            for paths in ([], ["README.md", "docs/evidence.md", ".github/instructions.md"]):
                self.assertEqual(policy.select_scope(event_name, event, paths)[:2], (False, False))
            self.assertEqual(policy.select_scope(event_name, event, ["README.md", "tools/check.py"])[:2], (True, False))

    def test_malformed_event_cannot_select_reduced_coverage(self):
        for event, data in (("schedule", {}), ("pull_request", {}),
                            ("pull_request", {"pull_request": {"draft": "true"}})):
            with self.assertRaises((ValueError, KeyError)):
                policy.select_scope(event, data, [])

    def test_gate_rejects_failed_cancelled_missing_required_jobs(self):
        policy.validate_results("false", "false", "success", "skipped", "skipped")
        policy.validate_results("true", "false", "success", "success", "skipped")
        policy.validate_results("true", "true", "success", "success", "success")
        for full in ("false", "true"):
            for failed in ("failure", "cancelled", "skipped", ""):
                with self.assertRaises(ValueError):
                    policy.validate_results("true", full, failed, "success", "success")
                with self.assertRaises(ValueError):
                    policy.validate_results("true", full, "success", failed, "success")
            for windows in ("failure", "cancelled", "", "skipped" if full == "true" else "success"):
                with self.assertRaises(ValueError):
                    policy.validate_results("true", full, "success", "success", windows)
        with self.assertRaises(ValueError):
            policy.validate_results("true", "", "success", "success", "skipped")
        for run, full, scope, result in (("", "false", "success", "skipped"),
                                         ("false", "true", "success", "skipped"),
                                         ("false", "false", "failure", "skipped"),
                                         ("false", "false", "success", "success"),
                                         ("false", "false", "success", "failure"),
                                         ("false", "false", "success", "cancelled"),
                                         ("false", "false", "success", "")):
            with self.assertRaises(ValueError):
                policy.validate_lifecycle(run, full, scope, result)
            with self.assertRaises(ValueError):
                policy.validate_results(run, full, scope, result, "skipped")

    def test_deleted_platform_path_survives_rename_detection(self):
        import os
        with tempfile.TemporaryDirectory() as directory:
            old = os.getcwd()
            try:
                os.chdir(directory)
                def git(*args):
                    env = {k: v for k, v in os.environ.items() if k not in ("GIT_DIR", "GIT_WORK_TREE", "GIT_INDEX_FILE")}
                    return subprocess.check_output(["git", *args], text=True, env=env).strip()
                git("init", "-q"); git("config", "user.email", "test@example.invalid"); git("config", "user.name", "CI test")
                path = Path("src/env/old.h"); path.parent.mkdir(parents=True); path.write_text("platform\n")
                git("add", "."); git("commit", "-qm", "base"); base = git("rev-parse", "HEAD")
                path.rename("renamed.h"); git("add", "-A"); git("commit", "-qm", "rename"); head = git("rev-parse", "HEAD")
                event = {"pull_request": {"draft": True, "base": {"sha": base}, "head": {"sha": head}}}
                from unittest.mock import patch
                with patch.dict(os.environ, {k: v for k, v in os.environ.items() if k not in ("GIT_DIR", "GIT_WORK_TREE", "GIT_INDEX_FILE")}, clear=True):
                    paths = policy.changed_paths("pull_request", event)
                    self.assertEqual(policy.changed_paths("push", {"before": base, "after": head}), paths)
                    self.assertEqual(policy.changed_paths("merge_group", {"merge_group": {"base_sha": base, "head_sha": head}}), paths)
                self.assertEqual(set(paths), {"src/env/old.h", "renamed.h"})
                self.assertEqual(policy.select_scope("pull_request", event, paths)[:2], (True, True))
            finally:
                os.chdir(old)

    def test_actual_action_outputs_match_selected_jobs(self):
        import json
        import os
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as directory:
            event = Path(directory) / "event.json"
            output = Path(directory) / "output"
            summary = Path(directory) / "summary"
            for run, full, paths in ((False, False, ["docs/evidence.md"]),
                                    (True, False, ["src/game/backend/base/Rule.cpp"]),
                                    (True, True, ["docs/evidence.md"])):
                event.write_text(json.dumps({"pull_request": {"draft": False, "labels": [{"name": "ci:full"}] if full else []}}))
                output.write_text("")
                with patch.dict(os.environ, {"GITHUB_EVENT_NAME": "pull_request", "GITHUB_EVENT_PATH": str(event),
                                             "GITHUB_OUTPUT": str(output), "GITHUB_STEP_SUMMARY": str(summary)}):
                    with patch.object(policy, "changed_paths", return_value=paths):
                        policy.main()
                fields = dict(line.split("=", 1) for line in output.read_text().splitlines())
                self.assertEqual(fields["full"], str(full).lower())
                self.assertEqual(fields["run"], str(run).lower())
                self.assertEqual(json.loads(fields["unix"]), ["ubuntu-24.04", "macos-14"] if full else ["ubuntu-24.04"])
                self.assertEqual([row["os"] for row in json.loads(fields["lifecycle"])["include"]], json.loads(fields["unix"]))


if __name__ == "__main__":
    unittest.main()
