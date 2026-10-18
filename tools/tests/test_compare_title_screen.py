"""The title gate must detect tiny errors and must not normalize bad captures."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from PIL import Image


SCRIPT = Path(__file__).resolve().parents[1] / 'reference' / 'compare_title_screen.py'
SPEC = importlib.util.spec_from_file_location('compare_title_screen', SCRIPT)
comparison = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(comparison)


class TitleComparisonTests(unittest.TestCase):
    def test_cli_exact_match_and_one_pixel_outside_diagnostic_regions(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            reference, candidate = root / 'reference.png', root / 'candidate.png'
            image = Image.new('RGB', comparison.SIZE, (10, 20, 30))
            image.save(reference)
            for changed, expected_exit in [(False, 0), (True, 1)]:
                if changed:
                    image.putpixel((650, 100), (11, 20, 30))
                image.save(candidate)
                process = subprocess.run([
                    sys.executable, str(SCRIPT), '--reference', str(reference),
                    '--candidate', str(candidate), '--output-dir', str(root / 'diagnostics'),
                ], capture_output=True, text=True)
                self.assertEqual(process.returncode, expected_exit, process.stderr)
                report = json.loads(process.stdout)
                self.assertEqual(report['regions']['whole_screen']['changed_pixels'], int(changed))
                self.assertEqual(report['regions']['menu']['changed_pixels'], 0)
                self.assertEqual(report['passed'], not changed)

    def test_rejects_wrong_size_and_transparency(self):
        with tempfile.TemporaryDirectory() as directory:
            capture = Path(directory) / 'capture.png'
            for image, message in [
                (Image.new('RGB', (1920, 1080)), 'expected'),
                (Image.new('RGBA', comparison.SIZE, (10, 20, 30, 254)), 'transparent'),
            ]:
                image.save(capture)
                with self.assertRaisesRegex(ValueError, message):
                    comparison.load_capture(capture)

    def test_diagnostics_cannot_overwrite_input(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            reference = root / 'difference.png'
            Image.new('RGB', comparison.SIZE).save(reference)
            original = reference.read_bytes()
            with self.assertRaisesRegex(ValueError, 'overwrite'):
                comparison.compare(reference, reference, root)
            self.assertEqual(reference.read_bytes(), original)


if __name__ == '__main__':
    unittest.main()
