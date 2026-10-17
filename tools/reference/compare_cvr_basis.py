"""Compare the production integer converter with pinned original x87 outputs.

Reads an existing private report; does not execute the original or create a new
rounding oracle. No camera, frame interpretation or floating-point defaults.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

REPORT_HASH = '4da27525e353d93085a608a1f6243a8664e86bb0a985be0d9335196d2fc1a7cb'
EXE_HASH = '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01'
SOURCE_HASH = 'fae91916c120d3518dcb2ce6d776d04e7d29833695a871b5efc887845fd6a46a'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    args = parser.parse_args()
    raw = args.report.read_bytes()
    if hashlib.sha256(raw).hexdigest() != REPORT_HASH:
        raise ValueError('Original report hash mismatch')
    report = json.loads(raw)
    if report['executable_sha256'] != EXE_HASH or report['probe_sha256'] != SOURCE_HASH or len(report['cases']) != 41:
        raise ValueError('Unexpected original evidence profile')
    cases = report['cases']
    run = subprocess.run([str(args.probe.resolve())], input='\n'.join(' '.join(c['input_bits']) for c in cases) + '\n',
                         text=True, capture_output=True, timeout=60, check=True)
    if run.stderr or len(run.stdout.splitlines()) != len(cases):
        raise ValueError('Unexpected production probe output')
    failures = [{'case': case['name'], 'expected': case['original_qword_results'], 'actual': actual}
                for case, actual in zip(cases, run.stdout.splitlines())
                if actual != 'ok ' + ' '.join(map(str, case['original_qword_results']))]
    print(json.dumps({'report_sha256': REPORT_HASH, 'cases': len(cases), 'values': len(cases) * 9,
                      'matched_cases': len(cases) - len(failures), 'failures': failures}, indent=2))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
