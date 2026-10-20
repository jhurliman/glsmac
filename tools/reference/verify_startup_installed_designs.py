#!/usr/bin/env python3
"""Verify original design pools with owned alpha/labels text, without retaining it."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile


def section(text, name):
    lines = [line.split(';', 1)[0].strip() for line in text.splitlines()]
    result = []
    for line in lines[lines.index('#' + name) + 1:]:
        if line.startswith('#'):
            break
        if line:
            result.append(line)
    return result


def literal(text):
    return '"' + ''.join('\\x%02x' % b for b in text.encode('cp1252')) + '"'


def braces(values):
    return '{' + ','.join(braces(v) if isinstance(v, (tuple, list)) else
                          str(v).lower() if isinstance(v, bool) else str(v) for v in values) + '}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory', type=Path, required=True)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    profile = json.loads((root / 'docs/behavior/startup-design-source-profile.json').read_text())
    files = {p.name.casefold(): p for p in args.asset_directory.iterdir() if p.is_file()}

    def read(name, key):
        raw = files[name.casefold()].read_bytes()
        if hashlib.sha256(raw).hexdigest() != profile[key]:
            raise ValueError('Owned design source hash differs: ' + name)
        return raw.decode('cp1252')

    alpha = read('alpha.txt', 'alpha_sha256')
    tables = {key: [[v.strip() for v in row.split(',')] for row in section(alpha, key)]
              for key in ['CHASSIS', 'WEAPONS', 'DEFENSES', 'ABILITIES', 'TECHNOLOGY', 'UNITS']}
    declared = int(tables['UNITS'].pop(0)[0])
    if declared != len(tables['UNITS']) or declared != 14:
        raise ValueError('Owned predefined row count differs')
    ch, we, ar, ab = (tables[k] for k in ['CHASSIS', 'WEAPONS', 'DEFENSES', 'ABILITIES'])
    components = [[[int(x[8]), int(x[9]), bool(int(x[11])), int(x[12]), int(x[13])] for x in ch],
                  [[int(x[2]), int(x[3]), int(x[4])] for x in we],
                  [[int(x[2]), int(x[4])] for x in ar],
                  [[int(x[1]), int(x[4], 2)] for x in ab]]
    technology = [row[1] for row in tables['TECHNOLOGY']]

    def tech(value):
        return -1 if value.lower() == 'none' else -2 if value.lower() == 'disable' else technology.index(value)

    rows = []
    for row in tables['UNITS']:
        values = [literal(row[0]), str([x[0] for x in ch].index(row[1])),
                  str([x[1] for x in we].index(row[2])), str([x[1] for x in ar].index(row[3]))]
        values += [str(int(row[i])) for i in [4, 5, 6]]
        values += [str(tech(row[7])), str(int(row[8])), str(int(row[9], 2)) + 'u']
        rows.append('{' + ','.join(values) + '}')
    labels = section(read('labels.txt', 'labels_sha256'), 'LABELS')[1:]
    names = [ar[1][1], ch[0][4], ch[0][6], labels[profile['scout_label_index']], ch[1][2]]
    header = ('#include "game/backend/unit/SmacPredefinedDesigns.h"\nnamespace owned_designs {\n'
              'inline game::backend::unit::SmacDesignComponents Components(){return {'
              + ','.join('{' + braces(v) + '}' for v in components) + '};}\n'
              'inline std::vector<game::backend::unit::SmacPredefinedDesign> Rows(){return {'
              + ','.join(rows) + '};}\n'
              'inline game::backend::unit::SmacEarlyDesignNames Names(){return {'
              + ','.join(map(literal, names)) + ',' + str(int(ar[1][2])) + '};}\n}\n')
    with tempfile.TemporaryDirectory(prefix='glsmac-designs-') as directory:
        temporary = Path(directory)
        (temporary / 'OwnedDesigns.h').write_bytes(header.encode())
        binary = temporary / 'design-probe'
        sources = ['tools/reference/startup_installed_designs_probe.cpp',
                   'src/game/backend/unit/SmacPredefinedDesigns.cpp',
                   'src/game/backend/unit/SmacOpeningDesigns.cpp',
                   'src/game/backend/faction/SmacStartupCapability.cpp']
        command = [args.cxx, '-std=c++17', '-O1', '-g', '-I' + str(root / 'src'), '-I' + str(temporary)]
        command += [str(root / source) for source in sources] + ['-o', str(binary)]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        subprocess.run(command, check=True)
        env = os.environ.copy()
        if args.sanitize:
            env['UBSAN_OPTIONS'] = 'halt_on_error=1'
        subprocess.run([str(binary)], env=env, check=True)
    print('Verified owned alpha/labels hashes; native manifest ' + profile['manifest_sha256'])


if __name__ == '__main__':
    main()
