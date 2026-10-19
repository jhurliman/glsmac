"""Verify C++ named quad submissions against frozen original caller commands.

Uses original captured base vertices/scalars as inputs. This checks the art
selector, not height reconstruction, raster output, or live renderer admission.
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
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    frozen=json.loads((root/'docs/behavior/named-landmark-quad-marker-art-cases.json').read_text())
    material=json.loads((root/'docs/behavior/landmark-material-cases.json').read_text())
    cases=frozen['producer_cases']+material['producer_cases']
    crops={(1+57*i,y,56,56) for y in [628,685] for i in range(3)}
    crops.update((768+57*(i%4),15+57*(i//4),56,56) for i in range(8))
    inputs=[];expected=[]
    for case in cases:
        spec=case['input'];land=spec['band']>=3
        points=[[576,288],[528,288],[576,264],[624,288],[576,312]];lighting=[0]*5
        if land:
            first,second,third=case['commands'][:3]
            points=[first['vertices'][:2],first['vertices'][2:4],first['vertices'][4:6],
                    second['vertices'][4:6],third['vertices'][4:6]]
            lighting=first['values'][:3]+[second['values'][2],third['values'][2]]
        row=[spec.get('landmark',0),int(land)]+[v for p in points for v in p]+lighting+[spec.get('flags',0x221),spec.get('remembered_items',spec['items'])]
        patches={(p[0],p[1]):p[2] for p in spec.get('patch',[])}
        words={(dx,dy):word for dx,dy,word in spec.get('neighbor_landmarks',[])}
        for offset in material['tables']['land']['neighbor_offsets']:
            key=tuple(offset);row.extend([patches.get(key,spec['band']),words.get(key,0)])
        inputs.append(row)
        draws=[d for d in case['commands'] if tuple(d.get('source_crop',[])) in crops]
        values=[len(draws)]
        for draw in draws:
            unlit=draw['mode']=='keyed_unlit'
            values.extend(draw['source_crop']+[int(unlit)]+draw['shade_limits'])
            for vertex in range(4):
                values.extend(draw['vertices'][2*vertex:2*vertex+2]+draw['uv'][2*vertex:2*vertex+2]
                              +[0 if unlit else draw['values'][vertex]])
        for banks in [{'0x781828','0x781854'}, {'0x781d30','0x781d5c','0x781d88','0x781db4','0x781de0','0x781e0c'}]:
            markers=[d for d in case['commands'] if d.get('bank') in banks]
            if len(markers)>1:raise RuntimeError('Unexpected multiple markers in one stage')
            values.append(int(bool(markers)))
            if markers:values.extend(markers[0]['binding']['crop'])
        jungle=[d for d in case['commands'] if d.get('source_crop') in material['tables']['jungle_crops']]
        if len(jungle)>1:raise RuntimeError('Unexpected multiple Jungle submissions')
        values.append(int(bool(jungle)))
        if jungle:
            draw=jungle[0];values.extend(draw['source_crop'])
            for vertex in range(4):
                values.extend(draw['vertices'][2*vertex:2*vertex+2]+draw['uv'][2*vertex:2*vertex+2])
        dunes=land and case['commands'][0].get('source_crop')==[229,1,56,56]
        values.append(int(dunes))
        if dunes:values.extend([229,1,56,56])
        expected.append(values)
    with tempfile.TemporaryDirectory(prefix='glsmac-landmark-art-') as directory:
        binary=Path(directory)/'probe'
        command=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),
                 str(root/'tools/reference/landmark_art_probe.cpp'),'-o',str(binary)]
        if args.sanitize:command.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])
        subprocess.run(command,check=True)
        result=subprocess.run([str(binary)],input=str(len(inputs))+'\n'+'\n'.join(' '.join(map(str,c)) for c in inputs),
                              text=True,capture_output=True,check=True)
        actual=[list(map(int,line.split())) for line in result.stdout.splitlines()]
        if len(actual)!=len(expected):raise RuntimeError('Truncated selector output')
        for index,(a,e) in enumerate(zip(actual,expected)):
            if a!=e:raise RuntimeError(f"{cases[index]['input']['id']}: expected {e}, got {a}")
    print(f'PASS: {len(inputs)} original named-art contexts; quad crop/order/geometry/UV/scalars/shader/limits marker stages, Jungle and Dunes')


if __name__=='__main__':main()
