"""Compare two native 1024x768 SMAC title captures without alignment or tolerance.

Requires Pillow. Captures and generated diagnostics contain original artwork;
keep them outside Git. A pixel match alone does not authenticate either capture.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops


SIZE = (1024, 768)
REGIONS = {
    'whole_screen': (0, 0, 1024, 768),
    'background_left': (0, 0, 630, 768),
    'menu': (670, 240, 1024, 768),
}


def load_capture(path):
    with Image.open(path) as image:
        if image.size != SIZE:
            raise ValueError(f'{path}: expected {SIZE}, got {image.size}; crop the actual content explicitly')
        rgba = image.convert('RGBA')
        if rgba.getchannel('A').getextrema() != (255, 255):
            raise ValueError(f'{path}: capture contains transparent pixels')
        return rgba.convert('RGB')


def compare(reference, candidate, output_dir):
    inputs = {reference.resolve(), candidate.resolve()}
    for name in ('difference.png', 'overlay.png', 'comparison.json'):
        if (output_dir / name).resolve() in inputs:
            raise ValueError('diagnostic output would overwrite an input capture')
    original = load_capture(reference)
    actual = load_capture(candidate)
    difference = ImageChops.difference(original, actual)
    regions = {}
    for name, box in REGIONS.items():
        pixels = list(difference.crop(box).getdata())
        regions[name] = {
            'box': box,
            'pixels': len(pixels),
            'changed_pixels': sum(pixel != (0, 0, 0) for pixel in pixels),
            'mean_absolute_channel_error': sum(sum(pixel) for pixel in pixels) / (3 * len(pixels)),
        }
    report = {
        'size': SIZE,
        'reference_sha256': hashlib.sha256(reference.read_bytes()).hexdigest(),
        'candidate_sha256': hashlib.sha256(candidate.read_bytes()).hexdigest(),
        'regions': regions,
        'passed': regions['whole_screen']['changed_pixels'] == 0,
        'policy': 'Exact whole-screen RGB equality. No masks, tolerance, resizing or alignment.',
    }
    output_dir.mkdir(parents=True, exist_ok=True)
    difference.save(output_dir / 'difference.png')
    Image.blend(original, actual, 0.5).save(output_dir / 'overlay.png')
    (output_dir / 'comparison.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', required=True, type=Path)
    parser.add_argument('--candidate', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    args = parser.parse_args()
    try:
        report = compare(args.reference, args.candidate, args.output_dir)
    except (OSError, ValueError) as error:
        parser.exit(2, f'{error}\n')
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
