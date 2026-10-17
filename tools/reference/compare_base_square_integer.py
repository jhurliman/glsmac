"""Compare direct center returns against the pinned original signed32 cases."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

FIXTURE_SHA = 'c92671c1dbd7fe16d403109bf78d801d07e34363284ee684de212ecea032c059'


def fields(value):
    return [
        *value['base_square_rules'], *value['bonus_square_rules'], *value['recycling_rules'],
        value['land'], ['none', 'nutrient', 'mineral', 'energy'].index(value['resource_bonus']),
        *value['landmark_bonus'], value['river'], value['hq'],
        value['recycling_tanks'], value['pressure_dome'], value['economy'],
        value['golden_age'], value['faction_base_count'], value['planetary_governor'],
        value['merchant_exchange_here'], *value['local_increase'], *value['local_decrease'],
        value['atmospheric_dust'], value['solar_multiplier_active'],
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', required=True, type=Path)
    parser.add_argument('--cases', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    raw = args.cases.read_bytes()
    if hashlib.sha256(raw).hexdigest() != FIXTURE_SHA:
        raise ValueError('Unexpected original signed32 fixture')
    fixture = json.loads(raw)
    cases = fixture['cases']
    if fixture['research_id'] != 'RE-SMAC-BASE-INTEGER-001' or len(cases) != 1389:
        raise ValueError('Unexpected reference case profile')
    rows = [fields({**fixture['defaults'], **case['overrides']}) for case in cases]
    if any(len(row) != 31 for row in rows):
        raise ValueError('Invalid probe input extent')
    data = ''.join(' '.join(str(int(value)) for value in row) + '\n' for row in rows)
    run = subprocess.run([str(args.probe.resolve())], input=data, capture_output=True,
                         text=True, check=True, timeout=60)
    results = [list(map(int, line.split())) for line in run.stdout.splitlines()]
    if run.stderr or len(results) != len(cases) or any(len(row) != 3 for row in results):
        raise ValueError('Unexpected probe output')
    failures = [{'id': case['id'], 'expected': case['expected'], 'actual': actual}
                for case, actual in zip(cases, results) if actual != case['expected']]
    result = {'fixture_sha256': FIXTURE_SHA,
              'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest(),
              'scenarios': len(cases), 'returns': len(cases) * 3, 'failures': failures}
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
