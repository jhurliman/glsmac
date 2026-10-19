"""Check resource/Pod/Monolith selection using production resource/site queries.

Compares original captured caller selections; does not verify live map authority
or frame assembly. Art assets are not needed for this selector check.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    frozen=json.loads((root/'docs/behavior/resource-pod-monolith-art-cases.json').read_text())
    inputs=[];expected=[];stages={'resource':0,'pod':1,'monolith':2}
    for case in frozen['producer_cases']:
        spec=case['input'];x,y=spec.get('tile',[40,40])
        inputs.append([x,y,spec.get('center_band',spec['band']),spec['items'],spec.get('pattern',0),
                       int(spec.get('automatic',False)),spec.get('flags',0x221),spec.get('zoom',0)])
        picks=[p for p in case['selection'] if p['kind'] in stages]
        row=[len(picks)]
        for pick in picks:
            row.extend([stages[pick['kind']],int(pick['kind']!='pod')]+frozen['bindings'][pick['bank']]['crop'])
        expected.append(row)
    # The caller's separate zoom gate must avoid every resource/site display.
    inputs.extend([[40,40,3,0x10002400,0,0,0x200,z] for z in [-8,-9]])
    expected.extend([[0],[0]])
    with tempfile.TemporaryDirectory(prefix='glsmac-resource-art-') as directory:
        binary=Path(directory)/'probe'
        sources=['tools/reference/resource_art_probe.cpp','src/game/backend/resource/SmacResourceBonus.cpp',
                 'src/game/backend/resource/SmacSiteSignal.cpp']
        command=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),
                 *[str(root/s) for s in sources],'-o',str(binary)]
        if args.sanitize:command.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])
        subprocess.run(command,check=True)
        result=subprocess.run([str(binary)],input=str(len(inputs))+'\n'+'\n'.join(' '.join(map(str,c)) for c in inputs),
                              text=True,capture_output=True,check=True)
        actual=[list(map(int,line.split())) for line in result.stdout.splitlines()]
        if len(actual)!=len(expected):raise RuntimeError('Truncated resource art output')
        for index,(a,e) in enumerate(zip(actual,expected)):
            if a!=e:raise RuntimeError(f'Resource context {index}: expected {e}, got {a}')
    print('PASS:52 original resource/Pod/Monolith callers and two zoom gates; actual production queries')


if __name__=='__main__':main()
