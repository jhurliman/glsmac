"""Compare the C++ landmark model to captured original executable returns."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    data=root/'docs/behavior'
    sys.path.insert(0,str(data))
    from terrain_geometry_reference import TerrainGeometry
    from landmark_geometry_reference import LandmarkGeometry,prepare_profiles
    frozen=json.loads((data/'landmark-height-shade-profiles-cases.json').read_text(encoding='utf-8'))
    contexts=[]
    expected=[]
    def context(case,requests,heights,words=None,tables=None):
        band=case.get('band',3)
        word=(case['detail']<<24)|case['bit']
        marks={(40,40):word} if words is None else words
        values=[band,int(case.get('coast',False)),case.get('height_step',25),len(marks)]
        for (x,y),w in marks.items():values.extend([x,y,w])
        values.append(len(requests))
        bands={(x,y):band for y in range(80) for x in range(y&1,80,2)}
        if case.get('coast'):bands[(39,39)]=2
        ordinary=TerrainGeometry(80,80,bands)
        reference=LandmarkGeometry(80,80,bands,words=marks,tables=tables or frozen['tables'])
        for request,height in zip(requests,heights):
            values.extend(request)
            expected.append((height,reference.shade(*request)-ordinary.shade(*request)))
        contexts.append(values)
    requests=[[40,40,k] for k in range(5)]
    for case in frozen['cases']:
        context(case,requests+requests,case['heights']+case['warm_heights'])
    for case in frozen['edges']['edges']:
        context(case,requests,case['heights'])
    for case in frozen['edges']['orders']:
        context(case,case['requests'],case['results'])
    # Suppression mutates shared stock tables, not the positive tile word.
    for suppression in frozen['suppression']:
        tables=prepare_profiles(frozen['tables'],[suppression['word']])
        for bit in [1,2]:
            for detail in range(49):
                case=dict(bit=bit,detail=detail,band=4)
                marks={(40,40):(detail<<24)|bit,(0,0):suppression['word']}
                bands={(x,y):4 for y in range(80) for x in range(y&1,80,2)}
                reference=LandmarkGeometry(80,80,bands,words=marks,tables=tables)
                context(case,requests,[reference.height(*r) for r in requests],marks,tables)
    with tempfile.TemporaryDirectory(prefix='glsmac-landmark-') as directory:
        executable=Path(directory)/'probe'
        command=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),
                 str(root/'tools/reference/landmark_geometry_probe.cpp'),'-o',str(executable)]
        if args.sanitize:command.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])
        subprocess.run(command,check=True)
        payload=str(len(contexts))+'\n'+'\n'.join(' '.join(map(str,c)) for c in contexts)+'\n'
        output=subprocess.run([str(executable)],input=payload,text=True,capture_output=True,check=True)
        actual=[tuple(map(int,line.split())) for line in output.stdout.splitlines()]
        if actual!=expected:
            for index,(a,e) in enumerate(zip(actual,expected)):
                if a!=e:raise RuntimeError(f'Query {index}: expected {e}, got {a}')
            raise RuntimeError(f'Output size {len(actual)} != {len(expected)}')
    print(f'PASS: {len(contexts)} C++ contexts, {len(expected)} height/shade queries')


if __name__=='__main__':main()
