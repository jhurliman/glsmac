#!/usr/bin/env python3
"""Check complete native naming buffers with owned text; retain no asset copies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile


def section(source, name):
    rows = [line.split(';', 1)[0].strip() for line in source.splitlines()]
    start = rows.index('#' + name) + 1
    result = []
    for row in rows[start:]:
        if row.startswith('#'):
            break
        if row:
            result.append(row.split(',', 1)[0].strip())
    return result


def literal(text):
    return '"' + ''.join('\\x%02x' % b for b in text.encode('cp1252')) + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory', type=Path, required=True)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    profile = json.loads((root / 'docs/behavior/startup-base-name-source-profile.json').read_text())
    files = {p.name.casefold(): p for p in args.asset_directory.iterdir() if p.is_file()}

    def read(name, expected):
        raw = files[name.casefold()].read_bytes()
        if hashlib.sha256(raw).hexdigest() != expected:
            raise ValueError('Owned naming source hash differs: ' + name)
        return raw.decode('cp1252')

    generic = section(read('BASENAME.txt', profile['generic_sha256']), 'GENERIC')
    catalogs = []
    for row in profile['factions']:
        text = read(row['file'] + '.txt', row['sha256'])
        first = next(line.split(';', 1)[0].strip() for line in text.splitlines()
                     if ',' in line and not line.lstrip().startswith(('#', ';')))
        noun = first.split(',')[2].strip()
        values = [literal(noun)]
        for names in (section(text, 'BASES'), section(text, 'WATERBASES'), generic):
            values.append('{' + ','.join(map(literal, names)) + '}')
        catalogs.append('{' + ','.join(values) + '}')
    # Temporary source and binary contain owned text and are removed together.
    with tempfile.TemporaryDirectory(prefix='glsmac-naming-') as directory:
        temporary = Path(directory)
        header = ('#include "game/backend/base/SmacBaseNaming.h"\n'
                  'inline std::array<game::backend::base::SmacBaseNameCatalog,7> Catalogs(){return {{'
                  + ','.join(catalogs) + '}};}\n')
        (temporary / 'OwnedNames.h').write_bytes(header.encode())
        source = root / 'tools/reference/startup_base_naming_probe.cpp'
        binary = temporary / 'naming-probe'
        command = [args.cxx, '-std=c++17', '-O1', '-g', '-I' + str(root / 'src'),
                   '-I' + str(temporary), str(source),
                   str(root / 'src/game/backend/base/SmacBaseNaming.cpp'), '-o', str(binary)]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        subprocess.run(command, check=True)
        env = os.environ.copy()
        if args.sanitize:
            env['UBSAN_OPTIONS'] = 'halt_on_error=1'
        subprocess.run([str(binary)], env=env, check=True)
    print('Verified eight owned source hashes; native manifest ' + profile['manifest_sha256'])


if __name__ == '__main__':
    main()
