"""Compare production dimensioned unknown rendering to captured original zoom frames."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--planet',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    data=json.loads((root/'docs/behavior/native-bitmap-zoom-cases.json').read_text())
    pinned=json.loads((root/'docs/behavior/planet-negative-landmark-full-cases.json').read_text())['asset_sha256']
    sha=lambda b:hashlib.sha256(b).hexdigest()
    if sha(args.planet.read_bytes())!=pinned:raise ValueError('Planet asset differs')
    groups=[data['schedules'][i:i+3] for i in range(0,9,3)]
    lines=[str(len(groups))]
    for cases in groups:
        profile=cases[0]['profile'];w,h=profile.get('map_size',[80,80]);vw,vh=profile.get('viewport',[1024,511])
        known=[[(22+dx)%w,38+dy] for dx,dy in data['supplied21_offsets']] if profile['knowledge']=='21' else []
        lines.append(' '.join(map(str,[w,h,vw,vh,profile['zoom'],int(not profile.get('flat',False)),len(known)])))
        lines.extend(' '.join(map(str,p)) for p in known)
        lines.extend(' '.join(map(str,c['after']['camera'])) for c in cases)
    sources=['tools/reference/native_bitmap_zoom_probe.cpp','src/game/frontend/NativeTerrainRenderer.cpp',
             'src/game/frontend/SmacTerrainRaster.cpp','src/game/frontend/SmacLandMaterial.cpp',
             'src/game/frontend/SmacWaterCoast.cpp','src/game/frontend/SmacTerritoryBoundary.cpp',
             'src/game/frontend/SmacRiver.cpp','src/game/frontend/SmacRoadTube.cpp',
             'src/game/frontend/SmacDevelopedOverlay.cpp','src/game/backend/map/SmacTerrainLighting.cpp',
             'src/game/backend/resource/SmacResourceBonus.cpp','src/game/backend/resource/SmacSiteSignal.cpp']
    args.out.mkdir(parents=True,exist_ok=True);exe=args.out/'probe'
    command=['g++','-std=c++17','-O1','-g','-I',str(root/'src'),*[str(root/s) for s in sources],'-o',str(exe)]
    if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run(command,check=True)
    subprocess.run([str(exe),str(args.planet),str(args.out)],input='\n'.join(lines)+'\n',text=True,check=True)
    pixel_cases={c['input']['id']:c for c in data['pixels']};pixels=queries=0
    for group,cases in enumerate(groups):
        for frame,c in enumerate(cases):
            stem=args.out/f'{group}-{frame}';events=[]
            for line in Path(str(stem)+'.queries').read_text().splitlines():
                kind,x,y,v,a,b=map(int,line.split());events.append(dict(kind=['height','shade','projector'][kind],args=[x,y] if kind==2 else [x,y,v],result=[a,b] if kind==2 else a))
            if sha(json.dumps(events,separators=(',',':')).encode())!=c['query_sha256']:raise AssertionError((c['input']['id'],'queries'))
            if sha(Path(str(stem)+'.cache').read_bytes())!=c['semantic_cache_sha256']:raise AssertionError((c['input']['id'],'cache'))
            queries+=len(events)
            if c['input']['id'] in pixel_cases:
                raw=Path(str(stem)+'.indexed').read_bytes();target=pixel_cases[c['input']['id']]
                if sha(raw)!=target['visible_frame_sha256']:raise AssertionError((c['input']['id'],'pixels',sha(raw),target['visible_frame_sha256']))
                pixels+=len(raw)
            print('PASS',c['input']['id'])
    print(f'PASS: 9 original schedules, {queries} query returns, 6 complete unknown frames / {pixels} indexed pixels')


if __name__=='__main__':main()
