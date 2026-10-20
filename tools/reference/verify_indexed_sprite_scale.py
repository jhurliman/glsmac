"""Verify production indexed sprite scaling against105 original synthetic controls."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    data=json.loads((root/'docs/behavior/native-indexed-sprite-scale-cases.json').read_text())
    args.out.mkdir(parents=True,exist_ok=True);exe=args.out/'probe'
    command=['g++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'tools/reference/indexed_sprite_scale_probe.cpp'),'-o',str(exe)]
    if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run(command,check=True)
    lines=[str(len(data['cases'])),' '.join(map(str,data['shadow']))]
    for c in data['cases']:
        s=c['input'];raw=bytes.fromhex(c['source_hex'])
        fields=c['frame_size']+[c['pitch']]+c['source_size']+[c['source_pitch']]+s['ratio']+s['anchor']+s['offset']+s['clip']+[['keyed','shadow','solid'].index(s['mode']),len(raw)]
        lines.extend([' '.join(map(str,fields)),' '.join(map(str,raw))])
    output=subprocess.run([str(exe),str(args.out)],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    reports=[line.split() for line in output.stdout.splitlines()]
    if len(reports)!=len(data['cases']):raise AssertionError('Truncated probe reports')
    total=0;sha=lambda b:hashlib.sha256(b).hexdigest()
    for i,(c,report) in enumerate(zip(data['cases'],reports)):
        frame=(args.out/f'{i}.frame').read_bytes();stores=(args.out/f'{i}.stores').read_bytes()
        if sha(frame)!=c['frame_sha256'] or sha(stores)!=c['store_sha256']:raise AssertionError((c['input']['id'],'pixels or ordered stores'))
        rect=None if report[2]=='none' else list(map(int,report[2:]))
        if int(report[1])!=c['store_count'] or rect!=c['rect']:raise AssertionError((c['input']['id'],'bounds/count'))
        total+=len(stores)//5
    print(f'PASS: {len(data["cases"])} original sprite frames and {total} ordered stores')


if __name__=='__main__':main()
