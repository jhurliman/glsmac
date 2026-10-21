const dialog = #include('../default/ui/parts/game/popup/base_screen/workshop');
let windows = []; let commands = []; let listeners = {};
const node = (data) => {
    data.hidden = false; data.handlers = {}; data.children = [];
    data.hide = () => { data.hidden = true; }; data.show = () => { data.hidden = false; }; data.remove = () => {};
    data.on = (event, cb) => { data.handlers[event] = cb; };
    data.listen = (target, event, cb) => { listeners[event] = cb; };
    const child = (properties) => { const value = node(properties); data.children :+ value; return value; };
    data.surface = child; data.button = child; data.input = child; data.panel = child;
    if (!#is_defined(data.text)) { data.text = (properties) => { test.assert(!#is_defined(properties.width)); return child(properties); }; }
    return data;
};
dialog.init({ui: {root: {window: (data) => { const result = node(data); windows :+ result; return result; }}},
    game: {event: (name, data) => { commands :+ {name: name, data: data}; }}});
const base = {id: 701};
const selection = {expected_state: 'before', chassis: 'Infantry', weapon: 'Hand Weapons', reactor: 'Fission', scout_name: 'Scout', speed: 1, offense: 1, power: 1,
    armor: [{id: 0, name: 'No Armor', short_name: 'None', rating: 1, design_name: 'Scout', cost: 10, available: true}, {id: 1, name: 'Synthmetal', short_name: 'Synthmetal', rating: 2, design_name: 'Synthmetal Garrison', cost: 20, available: true}, {id: 2, name: 'Unknown', short_name: 'Unknown', rating: 3, design_name: '', cost: 0, available: false}]};
test.assert(windows[0].left == 216 && windows[0].top == 168 && windows[0].width == 623 && windows[0].height == 387 && windows[0].children[0].background == 'deswin.pcx:smacsoftware()');
dialog.open(base, selection); test.assert(dialog.active && !windows[0].hidden && dialog.selected_armor == 0);
dialog.open_picker(); test.assert(!windows[1].hidden && #sizeof(dialog.cards) == 3);
test.assert(dialog.cards[2].handlers.click({}) == true && dialog.selected_armor == 0 && !windows[1].hidden);
test.assert(dialog.cards[1].handlers.click({}) == true && dialog.selected_armor == 1 && windows[1].hidden);
test.assert(dialog.title.text == 'SYNTHMETAL GARRISON' && dialog.stats.text == '1-2-1' && dialog.cost.text == 'COST:  20' && dialog.chassis.text == 'INFANTRY' && dialog.armor_name.text == 'SYNTHMETAL' && dialog.armor_stat.text == 'Level 2');
dialog.submit(false); dialog.submit(false);
test.assert(dialog.pending && #sizeof(commands) == 1 && commands[0].name == 'commit_smac_workshop'
    && commands[0].data.base == base && commands[0].data.armor == 1 && commands[0].data.name == ''
    && commands[0].data.expected_state == 'before' && !commands[0].data.done);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, stale: false, failed: false, intrinsic_name_warning: false, done: false});
test.assert(dialog.active && !dialog.pending);
dialog.hide_view(); test.assert(!dialog.active && windows[0].hidden && windows[1].hidden && #sizeof(commands) == 1);
dialog.open(base, selection); dialog.submit(true);
listeners.smac_workshop_commit({base: {id: 702}}); test.assert(dialog.pending);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, stale: true}); test.assert(dialog.active && !dialog.pending);
dialog.submit(true); listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, stale: false, failed: true}); test.assert(dialog.active && !dialog.pending);
dialog.submit(true); listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, stale: false, failed: false, intrinsic_name_warning: true, done: true});
test.assert(!dialog.active && windows[0].hidden && dialog.draft_name == '');
dialog.open(base, selection); dialog.open_picker();
test.assert(windows[1].handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true && windows[1].hidden && dialog.active);
test.assert(windows[0].handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true && !dialog.active);
dialog.open(base, selection); listeners.turn_status({status: 'please_wait'}); test.assert(!dialog.active && windows[0].hidden);

// An older Done response must not close a newer session on the same Base.
dialog.open(base, selection); dialog.submit(true); const old_request = dialog.pending_id; dialog.hide_view();
dialog.open(base, selection); dialog.submit(false); const current_request = dialog.pending_id;
listeners.smac_workshop_commit({base: base, request_id: old_request, selection: selection, stale: false, failed: false, intrinsic_name_warning: false, done: true});
test.assert(dialog.active && dialog.pending && current_request != old_request);
listeners.smac_workshop_commit({base: base, request_id: current_request, selection: selection, stale: false, failed: false, intrinsic_name_warning: false, done: false});
test.assert(dialog.active && !dialog.pending);

const before_keys = #sizeof(commands);
test.assert(windows[0].handlers.keydown({modifiers: {shift: true}, code: 'ENTER'}) == true && #sizeof(commands) == before_keys);
test.assert(windows[0].handlers.keydown({modifiers: {}, code: 'ENTER'}) == true && dialog.pending && #sizeof(commands) == before_keys + 1 && commands[before_keys].data.done);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, stale: false, failed: false, intrinsic_name_warning: false, done: true});
test.assert(!dialog.active && windows[0].hidden);
