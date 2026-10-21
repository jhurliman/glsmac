"""Exact RGB pixel comparison; optional x/y/width/height crop on candidate."""
import argparse
import json
from PIL import Image


def compare(reference, candidate, crop=None):
    a = Image.open(reference).convert('RGB')
    b = Image.open(candidate).convert('RGB')
    if crop:
        x, y, width, height = crop
        if min(x, y) < 0 or min(width, height) <= 0 or (
                x + width > b.width or y + height > b.height):
            raise ValueError('Candidate crop is outside image bounds')
        b = b.crop((x, y, x + width, y + height))
    if a.size != b.size:
        raise ValueError(f'Image size mismatch: {a.size} versus {b.size}')
    pa, pb = a.tobytes(), b.tobytes()
    points = [(i % a.width, i // a.width)
              for i in range(a.width * a.height)
              if pa[3*i:3*i+3] != pb[3*i:3*i+3]]
    return dict(width=a.width, height=a.height, pixels=a.width*a.height,
                differing_pixels=len(points),
                bbox=([min(x for x, y in points), min(y for x, y in points),
                       max(x for x, y in points)+1, max(y for x, y in points)+1]
                      if points else None), first_differences=points[:8])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference')
    parser.add_argument('candidate')
    parser.add_argument('--candidate-crop', type=int, nargs=4,
                        metavar=('X', 'Y', 'WIDTH', 'HEIGHT'))
    args = parser.parse_args()
    result = compare(args.reference, args.candidate, args.candidate_crop)
    print(json.dumps(result))
    raise SystemExit(bool(result['differing_pixels']))
