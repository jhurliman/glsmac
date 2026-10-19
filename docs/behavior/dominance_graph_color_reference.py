"""Loaded native-slot graph colors and admitted canonical-startup mapping."""
CANONICAL_DEFINITION_KEYS=('GAIANS','HIVE','UNIV','MORGAN','SPARTANS','BELIEVE','PEACE')


def generated_tables(palette_pcx):
    from terrain_triangle_raster_reference import pcx_crop
    return [[pcx_crop(palette_pcx,(580+8*slot,y,1,1))[0] for slot in range(8)] for y in [23,31,39,47]]


def graph_color(native_slot,graph_key,normal,alternate):
    if not 1<=native_slot<=7:raise ValueError('History series use native slots1..7')
    return (alternate if graph_key=='SPARTANS' else normal)[native_slot]


def canonical_slots(participants,*,default_factions_order):
    """Participants: (simulationID, frozen canonical definition key), not labels."""
    if not default_factions_order:raise ValueError('Custom/native imports require explicit loaded-slot provenance')
    source={key:slot for slot,key in enumerate(CANONICAL_DEFINITION_KEYS,1)}
    result={}
    for simulation_id,definition_key in participants:
        if simulation_id in result:raise ValueError('Duplicate simulation ID')
        result[simulation_id]=source[definition_key]
    if len(set(result.values()))!=len(result):raise ValueError('Duplicate canonical faction slot')
    return result


def seven_series(loaded_slot_records,normal,alternate):
    """Build identity/color roster independently of current accepted participants."""
    if [r['native_slot'] for r in loaded_slot_records]!=list(range(1,8)):raise ValueError('Retain all seven loaded slots')
    return [dict(native_slot=r['native_slot'],definition_key=r['filename'],graph_key=r['graph_key'],color=graph_color(r['native_slot'],r['graph_key'],normal,alternate)) for r in loaded_slot_records]


def verify(cases,palette_pcx=None):
    normal,alternate=cases['produced_tables'][:2]
    assert tuple(r['filename'] for r in cases['cases'][0]['slots'])==CANONICAL_DEFINITION_KEYS
    if palette_pcx:assert generated_tables(palette_pcx)==cases['produced_tables']
    for c in cases['cases']:
        assert not c['table_writes']
        assert c['tables_after']==cases['produced_tables']
        expected=[r['color'] for r in seven_series(c['slots'],normal,alternate)]
        assert expected==[r['index'] for r in c['slots']]
    assert cases['sparse_slots']==cases['cases'][1]['slots']
    bindings=canonical_slots([(1,'UNIV'),(2,'GAIANS')],default_factions_order=True)
    assert bindings=={1:3,2:1}
    assert graph_color(bindings[1],'UNIV',normal,alternate)==255
    assert cases['cases'][1]['slots'][0]['filename']=='UNIV' and cases['cases'][1]['slots'][0]['index']==241
    return len(cases['producer_writes']),sum(len(c['slots']) for c in cases['cases'])+len(cases['sparse_slots'])


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--palette-pcx');a=p.parse_args()
    cases=json.loads(Path(__file__).with_name('dominance-graph-colors-cases.json').read_text())
    print('PASS native table stores and graph color selections:',verify(cases,a.palette_pcx))
