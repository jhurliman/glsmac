"""Verify production view selection and query scheduling, without injected queries."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--planet-map',required=True)
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    parser.add_argument('--full-features',action='store_true',help='Compare the original complete tile-return feature schedule')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2];data=root/'docs/behavior'
    pinned=json.loads((data/'planet-negative-landmark-full-cases.json').read_text())['asset_sha256']
    asset=Path(args.planet_map)
    if hashlib.sha256(asset.read_bytes()).hexdigest()!=pinned:raise ValueError('Planet asset differs from original receipt')
    case_file='viewport-feature-schedule-cases.json' if args.full_features else 'viewport-geometry-lifecycle-cases.json'
    cases=json.loads((data/case_file).read_text())['cases']
    expected=[];lines=[str(len(cases))];camera=[40,40];zoom=0;relief=25
    kinds={'height':0,'shade':1,'projector':2}
    for i,c in enumerate(cases):
        spec=c['input']
        if 'camera' in spec:camera=spec['camera']
        if 'pan' in spec:camera=spec['pan']
        if 'zoom' in spec:zoom=spec['zoom']
        if 'relief' in spec:relief=spec['relief']
        mode=0 if 'guard' in spec else 2 if 'pan' in spec else 1
        if args.full_features:
            camera=c['after']['camera'];zoom=c['after']['zoom'];relief=c['after']['step']
        lines.append(' '.join(map(str,camera+[zoom,relief,mode])))
        for e in c['events']:
            if e['kind'] not in kinds:continue
            a=e['args'];value=e['result'];value=value if isinstance(value,list) else [value]
            expected.append([i,kinds[e['kind']],a[0],a[1],a[2] if len(a)>2 else 0]+value)
    with tempfile.TemporaryDirectory(prefix='glsmac-native-view-') as directory:
        exe=Path(directory)/'probe'
        cmd=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'tools/reference/native_view_geometry_probe.cpp'),str(root/'src/game/backend/map/SmacTerrainLighting.cpp'),'-o',str(exe)]
        if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        subprocess.run(cmd,check=True)
        result=subprocess.run([str(exe),str(asset)],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
        actual=[list(map(int,line.split())) for line in result.stdout.splitlines()]
        if actual!=expected:
            for index,(a,e) in enumerate(zip(actual,expected)):
                if a!=e:raise RuntimeError(f'Query {index}: expected {e}, got {a}')
            raise RuntimeError(f'Expected {len(expected)} queries, got {len(actual)}')
    print(f'PASS: production selected and scheduled all {len(expected)} original viewport query returns')

if __name__=='__main__':main()
