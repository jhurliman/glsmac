"""Replay original executable map-size/viewport/zoom geometry through production C++."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    frozen=json.loads((root/'docs/behavior/native-camera-geometry-cases.json').read_text())
    lines=[str(len(frozen['cases']))];expected=[];labels=[]
    for c in frozen['cases']:
        g=c['geometry'];i=c['input'];p=c['projections']
        lines.append(' '.join(map(str,c['world']+c['viewport']+c['camera']+
                                  [i['zoom'],int(i.get('flat',False)),g['relief'],len(p)])))
        expected.append([g['unit']]+g['first']+g['span']+g['offset']);labels.append((i['id'],'layout'))
        for point in p:
            lines.append(' '.join(map(str,point['tile'])))
            expected.append(point['anchor']+point['heights']);labels.append((i['id'],point['tile']))
    lines.append(str(len(frozen['height_edges'])))
    for edge in frozen['height_edges']:
        lines.append(f"{edge['zoom']} {edge['band']}")
        for label in ['cold/warm','relief reset/cache']:
            expected.append([edge['cold'],edge['warm']]);labels.append((edge['zoom'],label))
    with tempfile.TemporaryDirectory(prefix='glsmac-native-camera-') as directory:
        exe=Path(directory)/'probe'
        command=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),
                 str(root/'tools/reference/native_camera_geometry_probe.cpp'),
                 str(root/'src/game/backend/map/SmacTerrainLighting.cpp'),'-o',str(exe)]
        if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        subprocess.run(command,check=True)
        output=subprocess.run([str(exe)],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
        actual=[list(map(int,line.split())) for line in output.stdout.splitlines()]
        if len(actual)!=len(expected):raise RuntimeError('Truncated probe output')
        for a,e,label in zip(actual,expected,labels):
            if a!=e:raise RuntimeError(f'{label}: expected {e}, got {a}')
    print(f'PASS: {len(frozen["cases"])} original geometry controls, {len(expected)} layout/projector/height receipts; unknown traversals and mask dimensions')


if __name__=='__main__':main()
