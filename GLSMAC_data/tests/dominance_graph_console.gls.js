// Actual middle-area and info-panel callers. DOM and queued responses are
// doubled here; native tests cover the real provider, palette and Surface.
const middle = #include('../default/ui/parts/game/bottom_bar/middle_area');
let listeners = {};
let buttons = {};
let width = 1024;
let requests = 0;
let latest = 0;
let available = true;
let draws = 0;
let last_frame = #undefined;
const node = (props) => {
    const el = props;
    el.visible = true;
    el.handlers = {};
    el.on = (event, callback) => { el.handlers[event] = callback; };
    el.listen = (object, event, callback) => { object.on(event, callback); };
    el.panel = node;
    el.listview = node;
    el.surface = node;
    el.button = (p) => { const button = node(p); buttons[p.text] = button; return button; };
    el.hide = () => { el.visible = false; };
    el.show = () => { el.visible = true; };
    el.smacdominancegraph = (frame) => { draws++; last_frame = frame; return true; };
    el.measure = () => { return {width: 50, height: 11}; };
    if (!#is_defined(el.text)) { el.text = node; }
    return el;
};
const style = (name) => {
    const result = {set: (p) => {}};
    result.extend = (base) => { return result; };
    return result;
};
const game = {
    get_player: () => { return {get_faction: () => { return {name: 'Fixture'}; }}; },
    get_smac_faction_summary: () => { return {integrity: 'Noble', might: 'Sufficient', council_votes: 27}; },
    get_bm: () => { return {on: (name, callback) => {}}; },
    on: (event, callback) => { listeners[event] = callback; },
    request_smac_dominance_graph: (request) => { requests++; latest = request; return available; },
};
middle.init({game: game, el: node({}), ui: {class: style, get_width: () => { return width; }}});
const info = middle.pp.pages.info_panels;
test.assert(requests == 1 && latest == 1 && info.graph_pending, 'Initial visible graph did not queue a real rebuild');
info.refresh_data(); info.refresh_data();
test.assert(requests == 1, 'Pending graph rebuilds were not coalesced');
const frame = {marker: 'detached response fixture'};
listeners.smac_dominance_graph({request_id: 1, frame: frame});
test.assert(draws == 1 && last_frame == frame && info.graph.visible && !info.history.visible && requests == 2,
    'Owned graph response or dirty follow-up was lost');
listeners.smac_dominance_graph({request_id: 1, frame: #undefined});
test.assert(draws == 1 && info.graph_pending && info.graph.visible, 'Stale response cleared current graph');
listeners.smac_dominance_graph({request_id: 2, frame: frame});
test.assert(draws == 2 && requests == 2 && !info.graph_pending);
test.assert(buttons['I'].handlers.off({}) && buttons['M'].handlers.on({}), 'M/I DOM callbacks did not return Bool');
listeners.turn({year: 2102});
test.assert(!info.graph_visible && requests == 2, 'Hidden I page invoked score-only refresh');
test.assert(buttons['M'].handlers.off({}) && buttons['I'].handlers.on({}));
test.assert(info.graph_visible && requests == 3, 'Showing I did not rebuild current row');
listeners.smac_dominance_graph({request_id: 3, frame: #undefined});
test.assert(!info.graph.visible && info.history.visible, 'Unavailable history retained a fake/stale graph');
available = false;
info.refresh_data();
test.assert(requests == 4 && !info.graph_pending && !info.graph.visible && info.history.visible);
width = 1280;
test.assert(info.page.handlers.resize({}));
test.assert(requests == 4 && !info.graph.visible, 'Unmeasured width was passed to native geometry');
width = 1024; available = true;
test.assert(info.page.handlers.resize({}));
test.assert(requests == 5 && info.graph_pending, 'Returning to measured geometry failed to request a new frame');
