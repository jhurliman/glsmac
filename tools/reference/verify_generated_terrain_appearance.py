#!/usr/bin/env python3
"""Check production terrain against original natural startup coast/unknown scenes."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys

from verify_native_terrain_appearance import png


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory', required=True)
    parser.add_argument('--output-directory', required=True)
    parser.add_argument('--generated-records', type=Path,
                        help='Optional complete original 3200x44 startup records; checked by hash')
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    receipt = json.loads((root / 'docs/behavior/generated-world-terrain-acceptance-cases.json').read_text())
    out = Path(args.output_directory).resolve()
    out.mkdir(parents=True, exist_ok=True)
    sha = lambda data: hashlib.sha256(data).hexdigest()
    for name, digest in receipt['asset_hashes'].items():
        if sha((Path(args.asset_directory) / name).read_bytes()) != digest:
            raise ValueError(f'Original asset differs: {name}')
    # The same compiled probe must retain both earlier Planet relief controls.
    command = [sys.executable, str(root / 'tools/reference/verify_native_terrain_appearance.py'),
               '--asset-directory', args.asset_directory, '--output-directory', str(out), '--cxx', args.cxx]
    if args.sanitize:
        command.append('--sanitize')
    subprocess.run(command, check=True)
    if args.generated_records:
        records = args.generated_records.read_bytes()
        if len(records) != 3200*44 or sha(records) != receipt['producer_tiles_sha256']:
            raise ValueError('Complete generated records differ from original startup')
        scope = 'complete original generated records'
    else:
        # Only this bounded scene: preserve every observed field/tile pair.
        # Zeroed unobserved fields are not a general map representation.
        fields = receipt['observed_inputs']
        values = bytes.fromhex(fields['hex'])
        if len(values) != fields['bytes'] or sha(values) != fields['sha256']:
            raise ValueError('Observed generated input envelope differs')
        records = bytearray(3200*44)
        p = 0
        for tile in fields['tile_indices']:
            for field in fields['field_offsets']:
                if not 0 <= tile < 3200 or not 0 <= field < 44:
                    raise ValueError('Invalid observed generated field index')
                records[44*tile+field] = values[p]
                p += 1
        if p != len(values):
            raise ValueError('Observed generated field count differs')
        scope = 'bounded observed-field envelope'
    (out / 'generated.bin').write_bytes(records)
    subprocess.run([str(out / 'probe'), str(out), 'generated-startup'], check=True)
    if sha((out / 'palette.rgb.bin').read_bytes()) != receipt['palette_rgb_sha256']:
        raise ValueError('Production common palette differs from original')
    for case in receipt['frames']:
        name = case['id']
        rgb = None
        for kind, bpp in [('indexed', 1), ('rgb', 3)]:
            data = (out / f'{name}.{kind}.bin').read_bytes()
            if len(data) != 1024*511*bpp or sha(data) != case[f'{kind}_sha256']:
                raise ValueError(f'{name}: production {kind} framebuffer differs from original')
            for region in case['regions']:
                left, top, right, bottom = region['rect']
                crop = b''.join(data[(y*1024+left)*bpp:(y*1024+right)*bpp] for y in range(top, bottom))
                if sha(crop) != region[f'{kind}_sha256']:
                    raise ValueError(f'{name}: {region["id"]} production {kind} pixels differ')
            if kind == 'rgb':
                rgb = data
        (out / f'{name}.png').write_bytes(png(1024, 511, rgb))
        print(f'PASS {name}: original whole-frame and natural coast/unknown boundary indexed/RGB pixels')
    print(f'PASS: 1046528 generated-scene indexed/RGB pixels using {scope}; original generation itself is not implemented by this check')


if __name__ == '__main__':
    main()
