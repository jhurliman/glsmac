"""Compare production raster bytes to a retained private original-execution report."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess

EXE_HASH = '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01'
PROBE_HASH = 'dee511e774ffa42cce36c8e63c4ff4820b42b3b2cc2831dc5bbf80d8de55c167'
REPORT_HASH = 'f87e27980b2c0407e1c8f9218b70dabb1ee550c920c9b9c11ac31b2ce829f4db'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def installed_asset(directory, filename):
    matches = [path for path in directory.iterdir()
               if path.is_file() and path.name.lower() == filename.lower()]
    require(len(matches) == 1, 'installed asset must have one case-insensitive match: ' + filename)
    return matches[0]


def fixture(case):
    def supplied(label):
        values = [bytes.fromhex(entry['bytes']) for entry in case['supplied_bytes'] if entry['label'] == label]
        require(len(values) == 1, 'missing/ambiguous supplied input: ' + label)
        return values[0]
    first = supplied('synthetic 11-bit lighting-index lookup, not original lighting')
    second = supplied('synthetic lighting/material lookup, not original palette')
    records = supplied('raw record stream')
    width, height, stride = case['size']
    color = supplied('color payload and canaries')[16:-16]
    depth = supplied('depth payload and canaries')[16:-16]
    require(len(first) == 2048 and len(color) == stride * height and len(depth) == 2 * stride * height, 'bad report extents')
    request = b'CVRIDX1\0' + struct.pack('<4I', width, height, stride, len(second)) + first + second + color + depth
    request += struct.pack('<3I', 1, bool(case['mode'] & 32), 1 if case['mode'] & 1 else 2)
    request += struct.pack('<12i', *(v for row in case['basis'] for v in row), *case['initial'])
    request += struct.pack('<I', case['source_records']) + records
    return request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original-report', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    raw = args.original_report.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == REPORT_HASH, 'original report hash mismatch')
    original = json.loads(raw)
    require(original['executable_sha256'] == EXE_HASH and original['probe_sha256'] == PROBE_HASH, 'unsupported report provenance')
    require(len(original['cases']) == 42 and len(original['negative_controls']) == 8, 'incomplete original report')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    results = []
    for i, case in enumerate(original['cases']):
        request = args.output_dir / f'{i:02d}.request'
        output = args.output_dir / f'{i:02d}.output'
        request.write_bytes(fixture(case))
        command = [str(args.probe.resolve()), str(request), str(output)]
        if 'installed_geometry' in case:
            metadata = case['installed_geometry']
            asset = installed_asset(args.assets, metadata['file'])
            require(hashlib.sha256(asset.read_bytes()).hexdigest() == metadata['file_sha256'], 'asset hash mismatch')
            require(metadata['object'] in ('EH_M1-Mask05', 'VIPTA00'), 'unsupported object selection')
            command.extend([str(asset), '0'])
        completed = subprocess.run(command, capture_output=True, text=True, timeout=15)
        require(completed.returncode == 0, case['name'] + ': ' + completed.stderr)
        data = output.read_bytes()
        require(data[:8] == b'CVROUT1\0' and len(data) >= 36, 'invalid production output')
        width, height, stride, processed, written = struct.unpack_from('<3I2Q', data, 8)
        require([width, height, stride] == case['size'], 'surface shape mismatch')
        cells = stride * height
        require(len(data) == 36 + cells * 3, 'output extent mismatch')
        require(processed == case['drawn_records'] and written == len(case['written_samples']), 'record/write count mismatch')
        color_hash = hashlib.sha256(data[36:36 + cells]).hexdigest()
        depth_hash = hashlib.sha256(data[36 + cells:]).hexdigest()
        require(color_hash == case['color_sha256'] and depth_hash == case['depth_sha256'], case['name'] + ': original pixel/depth mismatch')
        results.append({'name': case['name'], 'processed_records': processed, 'written_samples': written,
                        'color_sha256': color_hash, 'depth_sha256': depth_hash,
                        'production_asset_reader': 'installed_geometry' in case})
    report = {'original_report_sha256': hashlib.sha256(raw).hexdigest(), 'cases': results,
              'matched': len(results), 'processed_records': sum(r['processed_records'] for r in results),
              'written_samples': sum(r['written_samples'] for r in results)}
    (args.output_dir / 'comparison.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k != 'cases'}))


if __name__ == '__main__':
    main()
