const create = #include('_restored_minimap');
let frame = {kind: 'fixture-observer-frame'};
let reads = 0;
let listeners = {};
const game = {
    get_year: () => { return 2101; },
    get_turn_status: () => { return 'active'; },
    get_smac_credits: () => { return 0; },
    get_smac_minimap: () => { reads++; return frame; },
    on: (name, callback) => { listeners[name] = callback; },
};
const read = create(game);
let state = read();
test.assert(state.overview == frame && reads == 1, 'initial/restored console bypassed local minimap query');
test.assert(state.geometry.align == 'top right' && state.geometry.right == 5 && state.geometry.top == 88 && state.geometry.width == 239 && state.geometry.height == 119,
    'console overview did not use measured source rectangle');
frame = {kind: 'next-observer-frame'};
state.map_event('unit_preview');
test.assert((read()).overview == frame && reads == 2, 'movement preview retained stale geography');
state.map_event('base_preview');
test.assert(reads == 3);
listeners.turn({year: 2102});
test.assert(reads == 4);
frame = {kind: 'sight-only-frame'};
listeners.map_knowledge({});
test.assert((read()).overview == frame && reads == 5, 'sight-only authority notification retained stale overview');
frame = #undefined;
test.assert(state.world_click({}) == true, 'World control callback did not return Bool');
test.assert(!#is_defined((read()).overview.kind) && reads == 6, 'unavailable authority retained prior full-map pixels');
reads = 0;
const legacy = create(game);
test.assert(!#is_defined((legacy()).overview) && reads == 1, 'no-knowledge legacy initialization disabled its existing capture');
frame = {};
const guarded = create(game);
test.assert((guarded()).overview == frame, 'explicit unavailable knowledge frame fell back to unfiltered legacy capture');
