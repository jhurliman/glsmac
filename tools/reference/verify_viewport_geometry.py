"""Compare production cache returns across original redraw/pan/zoom/relief events.

The original event coordinates/order are inputs, not a production view-selector
proof. Purchased map bytes are supplied locally and never written to fixtures.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--planet-map',required=True)
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    data=root/'docs/behavior';sys.path.insert(0,str(data))
    from minimap_navigation_reference import view_geometry
    frozen=json.loads((data/'viewport-geometry-lifecycle-cases.json').read_text())
    pinned=json.loads((data/'planet-negative-landmark-full-cases.json').read_text())['asset_sha256']
    asset=Path(args.planet_map)
    if hashlib.sha256(asset.read_bytes()).hexdigest()!=pinned:raise ValueError('Planet asset differs from original receipt')
    lines=[str(len(frozen['cases']))];expected=[];labels=[]
    kinds={'height':0,'shade':1,'projector':2}
    for case in frozen['cases']:
        after=case['after'];geo=view_geometry([80,80],[1024,511],after['camera'],after['zoom'])
        if geo['first']!=after['first']:raise ValueError('View origin differs from original receipt')
        events=[e for e in case['events'] if e['kind'] in kinds]
        lines.append(' '.join(map(str,[after['zoom'],after['step']]+geo['first']+geo['span']+[geo['diamond_size'][0]//8,len(events)])))
        for event in events:
            a=event['args'];lines.append(' '.join(map(str,[kinds[event['kind']],a[0],a[1],a[2] if len(a)>2 else 0])))
            result=event['result'];expected.append(result if isinstance(result,list) else [result])
            labels.append((case['input']['id'],event['kind'],a))
    with tempfile.TemporaryDirectory(prefix='glsmac-viewport-geometry-') as directory:
        exe=Path(directory)/'probe'
        command=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),
                 str(root/'tools/reference/viewport_geometry_probe.cpp'),str(root/'src/game/backend/map/SmacTerrainLighting.cpp'),'-o',str(exe)]
        if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        subprocess.run(command,check=True)
        output=subprocess.run([str(exe),str(asset)],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
        actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
        if len(actual)!=len(expected):raise RuntimeError('Truncated viewport returns')
        for a,e,label in zip(actual,expected,labels):
            if a!=e:raise RuntimeError(f'{label}: expected {e}, got {a}')
    print(f'PASS: {len(frozen["cases"])} original view lifecycle controls, {len(expected)} height/shade/projector returns')

if __name__=='__main__':main()
