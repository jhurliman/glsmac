"""Compare production padded view/pan selection against original ordered events."""
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
    cases=json.loads((root/'docs/behavior/viewport-geometry-lifecycle-cases.json').read_text())['cases']
    lines=[str(len(cases))];expected=[];camera=[40,40];zoom=0;count=0
    for i,c in enumerate(cases):
        spec=c['input']
        if 'camera' in spec:camera=spec['camera']
        if 'pan' in spec:camera=spec['pan']
        if 'zoom' in spec:zoom=spec['zoom']
        mode=0 if 'guard' in spec else 2 if 'pan' in spec else 1
        lines.append(' '.join(map(str,camera+[zoom,mode])))
        expected.append([0,i]+c['after']['first'])
        if mode==2:
            passes=[e for e in c['events'] if e['kind']=='pass']
            if len(passes)!=1:raise ValueError('Ambiguous original paint boundary')
            expected.append([3,i]+passes[0]['regions'])
            prefix=c['events'][:c['events'].index(passes[0])]
            first=[e['args'] for e in prefix if e['kind']=='projector']
            if len(first)!=2:raise ValueError('Missing original preliminary projections')
            expected.append([4,i]+first[0]+first[1])
        for kind,tag in [('terrain',1),('objects',2)]:
            for e in c['events']:
                if e['kind']==kind:
                    expected.append([tag,i]+(e['tile'] if kind=='terrain' else e['args'][:2]));count+=1
    with tempfile.TemporaryDirectory(prefix='glsmac-view-selection-') as directory:
        exe=Path(directory)/'probe'
        cmd=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'tools/reference/viewport_selection_probe.cpp'),'-o',str(exe)]
        if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        subprocess.run(cmd,check=True)
        result=subprocess.run([str(exe)],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
        actual=[list(map(int,line.split())) for line in result.stdout.splitlines()]
        if actual!=expected:
            for a,e in zip(actual,expected):
                if a!=e:raise RuntimeError(f'Expected {e}, got {a}')
            raise RuntimeError('Truncated viewport selection')
    print(f'PASS: {len(cases)} original view controls, {count} ordered terrain/object selections and both pan regions/preprojections')

if __name__=='__main__':main()
