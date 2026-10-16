"""Synthetic-only tests; no purchased assets or running game are required."""

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "reference_manifest.py"
SPEC = importlib.util.spec_from_file_location("reference_manifest", SCRIPT)
reference_manifest = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(reference_manifest)


class ReferenceManifestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "game"
        self.root.mkdir()

    def put(self, name, content=b"synthetic fixture"):
        path = self.root / name
        path.write_bytes(content)
        return path

    def run_cli(self, *args):
        return subprocess.run(
            [sys.executable, str(SCRIPT), *map(str, args)],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_hashes_bytes_and_detects_both_profiles_case_insensitively(self):
        data = b"\x00\xff\r\nsynthetic data\n"
        self.put("ALPHA.TXT", data)
        self.put("alphax.txt")
        self.put("Terran.EXE")
        self.put("TERRANX.EXE")

        manifest = reference_manifest.build_manifest(self.root)

        self.assertEqual(manifest["files"]["alpha.txt"], {
            "path": "ALPHA.TXT",
            "size_bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        })
        self.assertTrue(manifest["profiles"]["smac"]["reference_pair_present"])
        self.assertTrue(manifest["profiles"]["smacx"]["reference_pair_present"])
        self.assertFalse(manifest["execution_verified"])

    def test_partial_installation_is_reported_without_claiming_playability(self):
        self.put("alphax.txt")

        manifest = reference_manifest.build_manifest(self.root)

        self.assertTrue(manifest["profiles"]["smacx"]["rules_present"])
        self.assertFalse(manifest["profiles"]["smacx"]["executable_present"])
        self.assertFalse(manifest["profiles"]["smacx"]["reference_pair_present"])
        self.assertFalse(manifest["profiles"]["smac"]["rules_present"])
        self.assertIn("terranx.exe", manifest["missing_known_files"])

    def test_indicators_are_metadata_only_and_unrelated_files_are_excluded(self):
        self.put("alpha.txt", b"private original rule text")
        self.put("DDRAW.DLL")
        self.put("terran_PRACX.exe")
        self.put("goggame-1207658936.info", b"private installer metadata")
        self.put("unrelated-personal-file.txt", b"personal information")
        (self.root / "saves").mkdir()
        (self.root / "saves" / "alpha.txt").write_bytes(b"not a reference file")

        manifest = reference_manifest.build_manifest(self.root)
        encoded = json.dumps(manifest)

        self.assertEqual(manifest["patch_indicators"], ["ddraw.dll", "terran_pracx.exe"])
        self.assertEqual(manifest["distribution_indicators"], ["goggame-1207658936.info"])
        for excluded in ("private original", "private installer", "personal information", "saves", "unrelated-personal-file.txt"):
            self.assertNotIn(excluded, encoded)

    def test_stdout_is_deterministic_and_input_is_unchanged(self):
        original = self.put("alpha.txt", b"synthetic rules\r\n")
        before = original.read_bytes()

        first = self.run_cli(self.root)
        second = self.run_cli(self.root)

        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(first.stdout, second.stdout)
        self.assertEqual(first.stderr, "")
        self.assertEqual(original.read_bytes(), before)
        self.assertEqual(list(self.root.iterdir()), [original])
        self.assertEqual(json.loads(first.stdout)["schema_version"], 1)

    def test_explicit_output_creates_parent_outside_installation(self):
        self.put("alpha.txt")
        output = Path(self.temp.name) / "reference" / "manifest.json"

        result = self.run_cli(self.root, "--output", output)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "")
        self.assertEqual(json.loads(output.read_text(encoding="utf-8"))["schema_version"], 1)

    def test_output_cannot_overwrite_or_add_reference_installation_files(self):
        original = self.put("alpha.txt", b"do not modify")
        for output in (original, self.root / "new-manifest.json"):
            with self.subTest(output=output):
                result = self.run_cli(self.root, "--output", output)
                self.assertEqual(result.returncode, 2)
                self.assertIn("outside the reference installation", result.stderr)
        self.assertEqual(original.read_bytes(), b"do not modify")
        self.assertFalse((self.root / "new-manifest.json").exists())

    def test_output_replaces_hardlink_without_changing_reference_file(self):
        original = self.put("alpha.txt", b"do not modify")
        output = Path(self.temp.name) / "manifest.json"
        os.link(original, output)

        result = self.run_cli(self.root, "--output", output)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(original.read_bytes(), b"do not modify")
        self.assertEqual(json.loads(output.read_text(encoding="utf-8"))["schema_version"], 1)
        self.assertFalse(output.samefile(original))

    def test_failed_replacement_preserves_output_and_removes_temporary_file(self):
        output = Path(self.temp.name) / "reference" / "manifest.json"
        output.parent.mkdir()
        output.write_text("previous manifest", encoding="utf-8")

        with mock.patch.object(reference_manifest.os, "replace", side_effect=OSError("replace failed")):
            with self.assertRaisesRegex(OSError, "replace failed"):
                reference_manifest._write_manifest(output, "{}\n")

        self.assertEqual(output.read_text(encoding="utf-8"), "previous manifest")
        self.assertEqual(list(output.parent.iterdir()), [output])

    def test_bad_paths_have_actionable_errors(self):
        for path in (self.root / "missing", self.put("not-a-directory.txt")):
            with self.subTest(path=path):
                result = self.run_cli(path)
                self.assertEqual(result.returncode, 2)
                self.assertIn("Pass the game folder", result.stderr)
                self.assertNotIn("Traceback", result.stderr)
        result = self.run_cli(self.root)
        self.assertEqual(result.returncode, 2)
        self.assertIn("not its parent or a launcher folder", result.stderr)

    def test_ambiguous_case_is_rejected_on_case_sensitive_filesystems(self):
        self.put("alpha.txt")
        self.put("ALPHA.TXT")
        if len(list(self.root.iterdir())) != 2:
            self.skipTest("Filesystem does not allow names differing only by case")
        with self.assertRaisesRegex(reference_manifest.ReferenceManifestError, "Ambiguous"):
            reference_manifest.build_manifest(self.root)


if __name__ == "__main__":
    unittest.main()
