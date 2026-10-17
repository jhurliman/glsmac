#!/usr/bin/env python3
"""Compare production frame assembly with a retained original-execution report.

Requires locally owned VI/Vipta00 files and probe_scout_composition.py's report;
never launches the original executable or publishes its payloads.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-report', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    raw = args.reference_report.read_bytes()
    reference = json.loads(raw)
    if reference['status'] != 'passed' or reference['executable_sha256'] != '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01':
        raise RuntimeError('reference is not a passed pinned SMAC composition report')
    assets = reference['asset_metadata']
    for asset in assets:
        if hashlib.sha256((args.assets / asset['file']).read_bytes()).hexdigest() != asset['sha256']:
            raise RuntimeError('asset hash mismatch: ' + asset['file'])
    comparisons = []
    for case in reference['cases']:
        if case['initial_count'] != 0 or any(obj['source'] not in [a['file'] for a in assets] for obj in case['objects']):
            continue  # Native fixtures separately exercise synthetic appends/capacity.
        names = list(dict.fromkeys(obj['source'] for obj in case['objects']))
        command = [str(args.probe.resolve())]
        for index, name in enumerate(names):
            command += [str(args.assets / name), str(case['calls'][index]['frame'])]
            command += [f'{bits:08x}' for bits in struct.unpack('<3I', bytes.fromhex(case['offset_bits']))]
        completed = subprocess.run(command, capture_output=True, text=True, timeout=10, check=True)
        actual = json.loads(completed.stdout)
        if actual['first_omitted'] is not None or actual['count'] != case['final_count'] or len(actual['records']) != len(case['objects']):
            raise RuntimeError('assembly completion/count mismatch: ' + case['name'])
        for output, expected in zip(actual['records'], case['objects']):
            index = names.index(expected['source'])
            wanted = {'selection': index, 'scene': index, 'object': expected['object_index'], 'frame': expected['selected_frame'],
                      'flag': int(expected['flag'], 16), 'location_bits': list(struct.unpack('<3I', bytes.fromhex(expected['location_bits']))),
                      'matrix_bits': list(struct.unpack('<9I', bytes.fromhex(expected['matrix_bits'])))}
            if any(output[key] != value for key, value in wanted.items()):
                raise RuntimeError(f"record mismatch in {case['name']}: {expected['object_index']}")
        comparisons.append({'case': case['name'], 'records': actual['count']})
    if len(comparisons) != 30:
        raise RuntimeError('expected all 30 installed-file direct/selector cases')
    report = {'status': 'passed', 'reference_sha256': hashlib.sha256(raw).hexdigest(),
              'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest(), 'cases': comparisons,
              'records': sum(case['records'] for case in comparisons)}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'status': report['status'], 'cases': len(comparisons), 'records': report['records']}))


if __name__ == '__main__':
    main()
