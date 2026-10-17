"""Compare the production explicit-input calculator with captured original output.

Reads the pinned private report. Does not execute the original, import the
arithmetic oracle or infer a camera/lighting/default configuration.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess

REPORT_HASH = '9e10d7d6f3a0ab8b919b5bafc130fe4dbe85aa00fb312a6da5f61f270cd24662'
EXE_HASH = '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01'
SOURCE_HASH = 'e9696bf1cfc6a9b88b458ca66866ceea9b3a0ea12a5ded97351fba0866c70db2'


def words(value):
    return [f'{v:08x}' for (v,) in struct.iter_unpack('<I', bytes.fromhex(value))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    args = parser.parse_args()
    raw = args.report.read_bytes()
    if hashlib.sha256(raw).hexdigest() != REPORT_HASH:
        raise ValueError('Original report hash mismatch')
    report = json.loads(raw)
    if report['executable_sha256'] != EXE_HASH or report['probe_sha256'] != SOURCE_HASH or len(report['cases']) != 44:
        raise ValueError('Unexpected original evidence profile')
    requests, expected = [], []
    for case in report['cases']:
        header = bytes.fromhex(case['header_bytes'])
        request = []
        for field in ('supplied_transform_bytes', 'supplied_translation_bytes', 'frame_matrix_bytes', 'frame_location_bytes'):
            request.extend(words(case[field]))
        request.append(f'{struct.unpack_from("<I", header, 8)[0]:08x}')
        request.extend(map(str, struct.unpack_from('<3h', header, 32)))
        requests.append(' '.join(request))
        values = ['ok']
        # These are observed stage-memory bytes and original signed64 helper
        # results, not fresh expected values from the original probe's oracle.
        values.extend(words(case['A']['checkpoint_bytes']['scaled_matrix']))
        values.extend(words(case['A']['checkpoint_bytes']['translated_location']))
        values.extend(words(case['B']['checkpoint_bytes']['origin_float']))
        values.extend(map(str, case['A']['original_qword_results']))
        values.extend(map(str, case['B']['original_qword_results']))
        expected.append(' '.join(values))
    run = subprocess.run([str(args.probe.resolve())], input='\n'.join(requests) + '\n',
                         text=True, capture_output=True, timeout=60, check=True)
    if run.stderr or len(run.stdout.splitlines()) != len(requests):
        raise ValueError('Unexpected production probe output')
    failures = [{'case': case['name'], 'expected': wanted, 'actual': actual}
                for case, wanted, actual in zip(report['cases'], expected, run.stdout.splitlines()) if wanted != actual]
    print(json.dumps({'report_sha256': REPORT_HASH, 'cases': len(requests), 'bit_patterns': len(requests) * 15,
        'integer_results': len(requests) * 12, 'matched_cases': len(requests) - len(failures), 'failures': failures}, indent=2))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
