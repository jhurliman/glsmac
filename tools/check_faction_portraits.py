#!/usr/bin/env python3
"""Compare real 1x UI captures with independently decoded stock portraits.

Requires Pillow and the user's installed game assets. This checks opaque
portrait RGB, not full-dialog parity with an original executable capture.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--captures', type=Path, required=True,
                        help='One <faction-sheet-stem>.png per stock profile')
    parser.add_argument('--cases', type=Path, default=Path(__file__).resolve().parents[1] /
                        'docs/behavior/setup-portrait-render-cases.json')
    parser.add_argument('--x', type=int, default=60)
    parser.add_argument('--y', type=int, default=60)
    args = parser.parse_args()
    cases = json.loads(args.cases.read_text(encoding='utf-8'))
    palette = Image.open(args.assets / 'palette.pcx').getpalette()
    if palette is None or len(palette) != 768:
        raise ValueError('Expected the indexed 256-color common palette')
    results = []
    for profile in cases['profiles']:
        name = Path(profile['file']).stem.lower()
        path = args.assets / (name + '.pcx')
        if hashlib.sha256(path.read_bytes()).hexdigest() != profile['sha256']:
            raise ValueError(f'{name}: asset differs from the pinned reference')
        source = Image.open(path)
        if source.mode != 'P' or source.size != (1024, 768):
            raise ValueError(f'{name}: expected the original indexed faction sheet')
        raw = list(source.crop((304, 457, 435, 614)).getdata())
        rgba = bytearray()
        for index in raw:
            rgba.extend((0, 0, 0, 0) if index == 255 else
                        (*[min(c, 235) for c in palette[index * 3:index * 3 + 3]], 255))
        digest = hashlib.sha256(rgba).hexdigest()
        if digest != profile['common_gamma100_rgba_sha256']:
            raise ValueError(f'{name}: decoded expectation differs from the source receipt')
        screen = Image.open(args.captures / (name + '.png')).convert('RGB')
        if args.x < 0 or args.y < 0 or args.x + 131 > screen.width or args.y + 157 > screen.height:
            raise ValueError(f'{name}: portrait rectangle is outside the capture')
        actual = list(screen.crop((args.x, args.y, args.x + 131, args.y + 157)).getdata())
        expected = list(Image.frombytes('RGBA', (131, 157), bytes(rgba)).convert('RGB').getdata())
        opaque = [p for p, index in enumerate(raw) if index != 255]
        mismatches = sum(actual[p] != expected[p] for p in opaque)
        results.append(dict(faction=name, opaque_pixels=len(opaque), rgb_mismatches=mismatches))
    print(json.dumps(results, indent=2))
    return int(any(result['rgb_mismatches'] for result in results))


if __name__ == '__main__':
    raise SystemExit(main())
