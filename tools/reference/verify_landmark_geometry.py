"""Check the portable landmark reference against frozen original-executable returns.

This verifies the reference contract, not the production renderer or screen parity.
No purchased assets or emulator installation are needed.
"""
import json
from pathlib import Path
import sys


def main():
    data = Path(__file__).resolve().parents[2] / 'docs/behavior'
    sys.path.insert(0, str(data))
    from landmark_geometry_reference import LandmarkGeometry, prepare_profiles

    frozen = json.loads((data / 'landmark-height-shade-profiles-cases.json').read_text(encoding='utf-8'))
    tables = frozen['tables']

    def geometry(case, band=None):
        bands = {(x, y): case['band'] if band is None else band
                 for y in range(80) for x in range(y & 1, 80, 2)}
        if case.get('coast'):
            bands[(39, 39)] = 2
        return LandmarkGeometry(80, 80, bands, height_step=case.get('height_step', 25),
                                words={(40, 40): (case['detail'] << 24) | case['bit']}, tables=tables)

    def equal(actual, expected, label):
        if actual != expected:
            raise RuntimeError(f'{label}: expected {expected}, got {actual}')

    for index, case in enumerate(frozen['cases']):
        model = geometry(case)
        equal([model.height(40, 40, k) for k in range(5)], case['heights'], f'height {index}')
        equal([model.shade(40, 40, k) for k in range(5)], case['shades'], f'shade {index}')
        equal(model.projector(40, 40, 576, 288), case['projection'], f'projector {index}')
        equal([model.height(40, 40, k) for k in range(5)], case['warm_heights'], f'warm cache {index}')
    for index, case in enumerate(frozen['suppression']):
        prepared = prepare_profiles(tables, [case['word']])
        equal({k: prepared['profiles'][k] for k in case['profiles']}, case['profiles'], f'preparation {index}')
    for index, case in enumerate(frozen['edges']['edges']):
        model = geometry(case)
        equal([model.height(40, 40, k) for k in range(5)], case['heights'], f'edge height {index}')
        equal([model.shade(40, 40, k) for k in range(5)], case['shades'], f'edge shade {index}')
    for index, case in enumerate(frozen['edges']['orders']):
        model = geometry(case, 3)
        equal([model.height(*q) for q in case['requests']], case['results'], f'shared corner order {index}')
    print('PASS: 686 height/shade/projector contexts, 112 edges, 16 preparations, 10 cache orders')


if __name__ == '__main__':
    main()
