"""Installed filename lookup is independent of filesystem case sensitivity."""
import importlib.util
from pathlib import Path
import tempfile
import unittest


SPEC = importlib.util.spec_from_file_location(
    'compare_cvr_pixels', Path(__file__).resolve().parents[1] / 'reference' / 'compare_cvr_pixels.py')
comparison = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(comparison)


class InstalledAssetTests(unittest.TestCase):
    def test_mixed_case_names_resolve_to_installed_file(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for installed, requested in [('VI.cvr', 'vi.cvr'), ('Vipta00.cvr', 'vipta00.cvr')]:
                path = root / installed
                path.write_bytes(b'synthetic')
                self.assertEqual(comparison.installed_asset(root, requested), path)

    def test_missing_file_and_directory_only_match_reject(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for create_directory in (False, True):
                if create_directory:
                    (root / 'VI.cvr').mkdir()
                with self.assertRaisesRegex(RuntimeError, 'one case-insensitive match'):
                    comparison.installed_asset(root, 'vi.cvr')

    def test_case_collision_rejects_even_with_exact_match(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'VI.cvr').write_bytes(b'first')
            (root / 'vi.cvr').write_bytes(b'second')
            if len(list(root.iterdir())) != 2:
                self.skipTest('filesystem cannot represent a case collision')
            with self.assertRaisesRegex(RuntimeError, 'one case-insensitive match'):
                comparison.installed_asset(root, 'vi.cvr')


if __name__ == '__main__':
    unittest.main()
