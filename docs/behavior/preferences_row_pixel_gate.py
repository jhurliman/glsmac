"""Compare the original 1024x768 ID8 checklist row with a port capture.

Usage: python preferences_row_pixel_gate.py ORIGINAL.png PORT.png
Supply the same checkbox state in both images. Owned reference pixels are
not distributed. This deliberately excludes the rest of the Preferences UI.
"""
import argparse
from PIL import Image, ImageChops


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference')
    parser.add_argument('candidate')
    args = parser.parse_args()
    with Image.open(args.reference) as source, Image.open(args.candidate) as target:
        if source.size != (1024, 768) or target.size != (1024, 768):
            parser.error('Expected native 1024x768 captures without resizing')
        region = (418, 184, 875, 208)
        a = source.convert('RGB').crop(region)
        b = target.convert('RGB').crop(region)
    difference = ImageChops.difference(a, b)
    pixels = difference.load()
    changed = sum(pixels[x, y] != (0, 0, 0)
                  for y in range(difference.height) for x in range(difference.width))
    print(f'ID8 complete457x24 row: {changed} differing pixels of10968')
    return int(changed != 0)


if __name__ == '__main__':
    raise SystemExit(main())
