#!/usr/bin/env python3
"""Compare production indexed terrain, software palette and RGB against original controls."""
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import struct
import subprocess
import sys
import zlib


def png(width, height, rgb):
    def block(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    rows = b''.join(b'\0' + rgb[y*width*3:(y+1)*width*3] for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + block(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
            + block(b'IDAT', zlib.compress(rows, 9)) + block(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory', required=True)
    parser.add_argument('--output-directory', required=True)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    receipt = json.loads((root / 'docs/behavior/terrain-appearance-acceptance-cases.json').read_text())
    assets, out = Path(args.asset_directory), Path(args.output_directory)
    out.mkdir(parents=True, exist_ok=True)
    sha = lambda data: hashlib.sha256(data).hexdigest()
    for name, digest in receipt['asset_hashes'].items():
        if sha((assets / name).read_bytes()) != digest:
            raise ValueError(f'Original asset differs: {name}')
    subprocess.run([sys.executable, str(root / 'tools/reference/capture_native_viewport.py'),
                    '--asset-directory', str(assets), '--output-directory', str(out),
                    '--variation-low16', str(receipt['map_base_low16']), '--initial-index', '65',
                    '--viewer', '3', '--prepare-only'], check=True)
    common = (assets / 'PALETTE.PCX').read_bytes()
    if common[-769] != 12:
        raise ValueError('Missing PCX common palette trailer')
    (out / 'common-palette.bin').write_bytes(common[-768:])
    sources = ['tools/reference/native_terrain_appearance_probe.cpp', 'src/game/frontend/NativeTerrainRenderer.cpp',
               'src/game/frontend/SmacTerrainRaster.cpp', 'src/game/frontend/SmacLandMaterial.cpp',
               'src/game/frontend/SmacWaterCoast.cpp', 'src/game/frontend/SmacTerritoryBoundary.cpp',
               'src/game/frontend/SmacRiver.cpp', 'src/game/frontend/SmacRoadTube.cpp',
               'src/game/frontend/SmacDevelopedOverlay.cpp', 'src/game/backend/map/SmacTerrainLighting.cpp',
               'src/game/backend/resource/SmacResourceBonus.cpp', 'src/game/backend/resource/SmacSiteSignal.cpp',
               'src/loader/texture/SmacEffect.cpp']
    sdl_flags = shlex.split(subprocess.check_output(['sdl2-config', '--cflags'], text=True))
    sdl_libs = shlex.split(subprocess.check_output(['sdl2-config', '--libs'], text=True))
    command = [args.cxx, '-std=c++17', '-O1', '-g', '-I', str(root / 'src'), *sdl_flags,
               *[str(root / name) for name in sources], *sdl_libs, '-o', str(out / 'probe')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    subprocess.run(command, check=True)
    subprocess.run([str(out / 'probe'), str(out)], check=True)
    if sha((out / 'palette.rgb.bin').read_bytes()) != receipt['palette_rgb_sha256']:
        raise ValueError('Production common software palette differs from original')
    for case in receipt['frames']:
        name = case['id']
        indexed, rgb = (out / f'{name}.indexed.bin').read_bytes(), (out / f'{name}.rgb.bin').read_bytes()
        if len(indexed) != 1024*511 or len(rgb) != 1024*511*3:
            raise ValueError('Unexpected terrain framebuffer format')
        for region in case['regions']:
            left, top, right, bottom = region['rect']
            for kind, data, bpp in [('indexed', indexed, 1), ('rgb', rgb, 3)]:
                crop = b''.join(data[(y*1024+left)*bpp:(y*1024+right)*bpp] for y in range(top, bottom))
                if sha(crop) != region[f'{kind}_sha256']:
                    raise ValueError(f'{name} {region["id"]}: production {kind} pixels differ from original')
        (out / f'{name}.png').write_bytes(png(1024, 511, rgb))
        print(f'PASS {name}: whole, west coast and east ridge indexed/RGB pixels')
    a, b = [(out / f'{name}.indexed.bin').read_bytes() for name in ['normal_relief', 'low_relief']]
    assert sum(x != y for x, y in zip(a, b)) == receipt['normal_low_different_indexed_pixels']
    print('PASS: 1046528 original indexed and RGB pixels; exact 256-entry software palette')


if __name__ == '__main__':
    main()
