"""Compare the complete owned Scout renderer with pinned original draw hashes.

Requires user-owned installed models and the retained public-input fixture.
No original probe code, camera matrices, lookup bytes or asset outputs are fed
to the renderer. The one target-culled case is reported separately: this API
produces an internal image and does not implement target clipping or copying.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import tempfile

from compare_cvr_pixels import installed_asset

CASES_SHA = '91d4a68b9cdc330741278d9a2ebc40b9d75706f7f9a2f1c0bd11378f2bdf7894'
EXE_SHA = '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01'
ASSETS = {
    'vi.cvr': 'd2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1',
    'viptr00.cvr': '572ca9d55ad49c2ab32a90ae8d3e1b1bc9a78e7901cf4387d26d0dde205a4cee',
}
LIGHTING_SHA = '81102740e2d482516c9d1b42fec5972c2ba8364cb597fd7492e82d1e287eec60'
RGBA_SHA = '8a48c86fc334faf5bb99fc59e543b10c8f5d08a09085b2b2cf20d520c5508884'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--cases', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    raw = args.cases.read_bytes()
    if sha(raw) != CASES_SHA:
        raise ValueError('Unexpected original view fixture')
    fixture = json.loads(raw)
    if fixture['executable_sha256'] != EXE_SHA or len(fixture['cases']) != 20:
        raise ValueError('Unexpected original evidence profile')
    assets = [installed_asset(args.assets, name) for name in ASSETS]
    for path, expected in zip(assets, ASSETS.values()):
        if sha(path.read_bytes()) != expected:
            raise ValueError(f'Unexpected installed asset: {path.name}')
    results, failures = [], []
    with tempfile.TemporaryDirectory(prefix='scout-render-') as temporary:
        output = Path(temporary) / 'pixels.bin'
        run = subprocess.run([str(args.probe.resolve()), '--lighting', str(output)],
                             text=True, capture_output=True, timeout=60, check=True)
        if run.stderr or len(output.read_bytes()) != 51456 or sha(output.read_bytes()) != LIGHTING_SHA:
            raise ValueError('Generated lighting table differs')
        for case in fixture['cases']:
            value = case['input']
            argv = [value['facing'], value['zoom'], *value['ratio'], *value['anchor']]
            run = subprocess.run([str(args.probe.resolve()), '--render', *map(str, assets), str(output),
                                  *map(str, argv)], text=True, capture_output=True, timeout=60, check=True)
            data = output.read_bytes()
            if run.stderr or len(data) != 458752:
                raise ValueError('Unexpected complete renderer output')
            stats = list(map(int, run.stdout.split()))
            if len(stats) != 6 or stats[3:5] != case['expected']['anchor'] or stats[-1] != len(data):
                raise ValueError('Unexpected renderer statistics or target anchor')
            color, depth, rgba = data[:65536], data[65536:196608], data[196608:]
            hashes = [sha(color), sha(depth), sha(rgba)]
            compared = case['kernels'] != 0
            if compared and (stats[0] != case['kernels'] or hashes[:2] != [case['indexed_sha256'], case['depth_sha256']]):
                failures.append(value['name'])
            if value['name'] == 'facing_0' and (hashes[2] != RGBA_SHA or stats[:3] != [65, 47483, 2627]):
                failures.append('baseline_keyed_rgba_or_counts')
            if any(rgba[i * 4 + 3] != (0 if index == 9 else 255) for i, index in enumerate(color)):
                failures.append(value['name'] + '_keyed_alpha')
            results.append({'name': value['name'], 'original_internal_compared': compared,
                            'indexed_sha256': hashes[0], 'depth_sha256': hashes[1], 'rgba_sha256': hashes[2],
                            'draws_records_writes': stats[:3], 'target_anchor': stats[3:5]})
    result = {'fixture_sha256': CASES_SHA, 'original_executable_sha256': EXE_SHA,
              'probe_sha256': sha(args.probe.read_bytes()), 'installed_assets': ASSETS,
              'generated_lighting_sha256': LIGHTING_SHA, 'original_drawing_views': 19,
              'metadata_views': 20, 'target_culled_original_excluded_from_pixel_comparison': 1,
              'failures': failures, 'results': results}
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ('original_drawing_views', 'metadata_views', 'failures')}))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
