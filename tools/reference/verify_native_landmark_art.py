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
    crops={(1+57*i,y,56,56) for y in [628,685] for i in range(3)}
    crops.update((768+57*(i%4),15+57*(i//4),56,56) for i in range(8))
    inputs=[];expected=[]
    for case in frozen['producer_cases']:
        spec=case['input'];land=spec['band']>=3
        points=[[576,288],[528,288],[576,264],[624,288],[576,312]];lighting=[0]*5
        if land:
            first,second,third=case['commands'][:3]
            points=[first['vertices'][:2],first['vertices'][2:4],first['vertices'][4:6],
                    second['vertices'][4:6],third['vertices'][4:6]]
            lighting=first['values'][:3]+[second['values'][2],third['values'][2]]
        inputs.append([spec.get('landmark',0),int(land)]+[v for p in points for v in p]+lighting)
        draws=[d for d in case['commands'] if tuple(d.get('source_crop',[])) in crops]
        values=[len(draws)]
        for draw in draws:
            unlit=draw['mode']=='keyed_unlit'
            values.extend(draw['source_crop']+[int(unlit)]+draw['shade_limits'])
            for vertex in range(4):
                values.extend(draw['vertices'][2*vertex:2*vertex+2]+draw['uv'][2*vertex:2*vertex+2]
                              +[0 if unlit else draw['values'][vertex]])
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
            if a!=e:raise RuntimeError(f"{frozen['producer_cases'][index]['input']['id']}: expected {e}, got {a}")
    print(f'PASS: {len(inputs)} original named-art contexts; quad crop/order/geometry/UV/scalars/shader/limits')


if __name__=='__main__':main()
