"""Compare explicit initialized Infantry against nine retained original draws.

Only installed assets and public view inputs enter the production probe. Faction
identity and the actor-remap control select evidence rows, not runtime tables.
The receipt's 59 scene submissions and 65 raster calls are different counters;
this comparator checks complete images and records the port's counters separately.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import tempfile

from compare_cvr_pixels import installed_asset

CASES_SHA = '9d24d7949babf7c92c67737ac0b4e5c9ff0d9f19770bad47e3b0dc2a02f2d1b8'
REPORT_SHA = 'a9eaf014607bf7aaf338ae4d30d8f52de49a7145eea01a967667025eea5318c1'
EXE_SHA = '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01'
ASSETS = {
    'vi.cvr': 'd2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1',
    'viptr00.cvr': '572ca9d55ad49c2ab32a90ae8d3e1b1bc9a78e7901cf4387d26d0dde205a4cee',
    'PALETTE.PCX': '54341b1bee31736997950e432ff3312a620f7cf722016c154098a4c78d28f866',
    'system.txt': 'cadbe5153d3e36fbb95638f6aec54ba929b6f60af56e782b38042fb835cb146f',
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for field in ('probe', 'assets', 'cases', 'report', 'output'):
        parser.add_argument('--' + field, type=Path, required=True)
    args = parser.parse_args()
    if sha(args.cases.read_bytes()) != CASES_SHA or sha(args.report.read_bytes()) != REPORT_SHA:
        raise ValueError('Unexpected retained initialized Infantry evidence')
    fixture = json.loads(args.cases.read_bytes())
    report = json.loads(args.report.read_bytes())
    if report['executable_sha256'] != EXE_SHA or report['fixture_sha256'] != CASES_SHA or report['total'] != 9 or report['passed'] != 9:
        raise ValueError('Unexpected original capture profile')
    if [row['id'] for row in fixture['cases']] != [row['id'] for row in report['results']]:
        raise ValueError('Original evidence order differs')
    assets = [installed_asset(args.assets, name) for name in ASSETS]
    for path, expected in zip(assets, ASSETS.values()):
        if sha(path.read_bytes()) != expected:
            raise ValueError('Unexpected installed input: ' + path.name)
    results, failures = [], []
    with tempfile.TemporaryDirectory(prefix='initialized-infantry-') as temporary:
        output = Path(temporary) / 'pixels.bin'
        run = subprocess.run([str(args.probe.resolve()), '--software-palette', str(assets[2]), str(output)],
                             capture_output=True, text=True, timeout=60, check=True)
        colors = output.read_bytes()
        if run.stderr or len(colors) != 768 or sha(colors) != fixture['palettes']['main_rgb_sha256']:
            raise ValueError('Generated main software palette differs')
        for case, original in zip(fixture['cases'], report['results']):
            expected = original['actual']
            if not original['passed'] or expected != case['expected']:
                raise ValueError('Fixture is not the retained actual result')
            value = case['inputs']
            argv = [value['facing'], value['zoom'], *fixture['defaults']['ratio'], *fixture['defaults']['anchor']]
            run = subprocess.run([str(args.probe.resolve()), '--render-initialized', *map(str, assets), str(output), *map(str, argv)],
                                 capture_output=True, text=True, timeout=60, check=True)
            data = output.read_bytes()
            stats = list(map(int, run.stdout.split()))
            if run.stderr or len(data) != 458752 or len(stats) != 6 or stats[-1] != len(data):
                raise ValueError('Unexpected production output framing')
            color, depth, rgba = data[:65536], data[65536:196608], data[196608:]
            hashes = [sha(color), sha(depth), sha(rgba)]
            if hashes != [expected['indexed_sha256'], expected['depth_sha256'], expected['key9_active_rgba_sha256']] or stats[3:5] != expected['target_copy_anchor']:
                failures.append(case['id'])
            independent = bytes(channel for index in color for channel in (*colors[index*3:index*3+3], 0 if index == 9 else 255))
            if rgba != independent:
                failures.append(case['id'] + '_palette_key9')
            results.append({'id': case['id'], 'indexed_sha256': hashes[0], 'depth_sha256': hashes[1], 'rgba_sha256': hashes[2],
                            'target_anchor': stats[3:5], 'port_draws_records_writes': stats[:3],
                            'original_frame_records': expected['frame_records'], 'original_raster_kernels': expected['raster_kernels']})
    result = {'fixture_sha256': CASES_SHA, 'report_sha256': REPORT_SHA, 'executable_sha256': EXE_SHA,
              'probe_sha256': sha(args.probe.read_bytes()), 'installed_assets': ASSETS,
              'original_drawing_cases': len(results), 'failures': failures, 'results': results}
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ('original_drawing_cases', 'failures')}))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
