"""Bounded ordinary zoom0 full-view terrain submissions,80x80/1024x511."""
from terrain_geometry_reference import planet_view_origin


def submissions(camera_x,camera_y,wrap_x=True):
    first_x,first_y=planet_view_origin(camera_x,camera_y,wrap_x)
    result=[]
    # Native view initializer:25 candidate rows,12 even/11 odd columns.
    for row in range(25):
        y=first_y+row
        for col in range(11 if row&1 else 12):
            unwrapped_x=first_x+(row&1)+2*col
            x=unwrapped_x
            if wrap_x:
                if x<0:x+=80
                elif x>=80:x-=80
            if not (0<=x<80 and 0<=y<80):continue
            result.append(dict(tile=[x,y],flat_center=[48*(unwrapped_x-first_x),24*row],record_offset=44*(40*y+x//2)))
    return result


def record_address(base,x,y):
    if not (0<=x<80 and 0<=y<80 and (x+y)%2==0):
        raise ValueError('Valid doubled-X80x80 tile required')
    return base+44*(40*y+x//2)


def material_variation(base,x,y):
    seed=record_address(base,x,y)&65535
    return (((seed*1664525+1013904223)&65535)*2)>>16


def verify(cases):
    count=0
    for case in cases['cases']:
        actual=case['submissions'];want=submissions(*case['camera'],not case['flat_map'])
        assert len(actual)==len(want),case['id']
        for a,b in zip(actual,want):
            for key in ['tile','flat_center','record_offset']:assert a[key]==b[key],(case['id'],key,a,b)
        count+=len(actual)
    return count


if __name__=='__main__':
    import json
    from pathlib import Path
    print('PASS',verify(json.loads(Path(__file__).with_name('terrain-view-order-cases.json').read_text())),'ordered native submissions')
