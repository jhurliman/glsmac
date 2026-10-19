const create = #include('_restored_minimap');
let detail = {kind: 'smac-detail-ordinary-v1'};
let listeners = {};
const game = {
    get_year: () => { return 2105; }, get_turn_status: () => { return 'active'; }, get_smac_credits: () => { return 15; },
    get_smac_minimap: () => { return {kind: 'world'}; }, get_smac_detail_minimap: () => { return detail; },
    on: (name, callback) => { listeners[name] = callback; },
};
const read = create(game);
let state = read();
test.assert(state.detail == detail && state.detail_visible, 'actual console did not expose supported separate Detail frame');
test.assert(state.detail_click({}) == true, 'Detail button callback must return Bool');
test.assert((read()).mode == 'detail', 'actual Detail control did not select its measured view');
listeners.turn({year: 2106});
test.assert((read()).detail == detail && !#is_defined((read()).mode), 'ordinary refresh forced a new mode/camera selection');
test.assert(state.world_click({}) == true && (read()).mode == 'world', 'World did not restore overview');
detail = #undefined;
listeners.map_knowledge({});
test.assert(!(read()).detail_visible && !#is_defined((read()).detail.kind), 'unsupported or retired authority retained stale Detail view');
