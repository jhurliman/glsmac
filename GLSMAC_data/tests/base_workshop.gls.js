const dialog = #include('../default/ui/parts/game/popup/base_screen/workshop');
let windows = []; let commands = []; let listeners = {};
const node = (data) => {
    data.hidden = false; data.handlers = {}; data.children = [];
    data.hide = () => { data.hidden = true; }; data.show = () => { data.hidden = false; }; data.remove = () => {};
    data.smacworkshoparmorpicker = (selected, rows) => { data.picker_selected = selected; data.picker_rows = rows; };
    data.measure = () => { return {width: 100}; };
    data.smacworkshopcomponents = (armor) => { data.components_armor = armor; };
    data.smacworkshopdesign = (chassis, weapon, armor, reactor, kind, faction, yaw) => { data.preview_armor = armor; data.preview_yaw = yaw; data.preview_faction = faction; };
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
const selection = {expected_state: 'before', chassis: 'Infantry', weapon: 'Hand Weapons', reactor: 'Fission', scout_name: 'Scout', faction_art: 'morgan.pcx', speed: 1, offense: 1, power: 1,
    armor: [{id: 0, name: 'No Armor', short_name: 'None', rating: 1, design_name: 'Scout', cost: 10, technology_available: true, available: true}, {id: 1, name: 'Synthmetal', short_name: 'Synthmetal', rating: 2, design_name: 'Synthmetal Garrison', cost: 20, technology_available: true, available: true}, {id: 2, name: 'Unknown', short_name: 'Unknown', rating: 3, design_name: '', cost: 0, technology_available: false, available: false}]};
test.assert(windows[0].left == 216 && windows[0].top == 168 && windows[0].width == 623 && windows[0].height == 387 && windows[0].children[0].background == 'deswin.pcx:smacsoftware()');
dialog.open(base, selection); test.assert(dialog.active && !windows[0].hidden && dialog.selected_armor == 0);
test.assert(dialog.model.preview_armor == 0 && dialog.model.preview_yaw == 0.0 && dialog.model.preview_faction == 'morgan.pcx');
dialog.open_picker(); test.assert(!windows[1].hidden && #sizeof(dialog.cards) == 3 && dialog.picker_canvas.picker_selected == 0 && #sizeof(dialog.picker_canvas.picker_rows) == 3 && #sizeof(dialog.cards[2].children) == 0);
test.assert(dialog.cards[2].handlers.click({}) == true && dialog.selected_armor == 0 && !windows[1].hidden);
test.assert(dialog.cards[1].handlers.click({}) == true && dialog.selected_armor == 1 && windows[1].hidden);
test.assert(dialog.model.preview_armor == 1 && dialog.model.preview_yaw == 0.0 && dialog.components.components_armor == 1);
test.assert(dialog.title.text == 'SYNTHMETAL GARRISON' && dialog.stats.text == '1-2-1' && dialog.cost.text == 'COST:  20' && dialog.chassis.text == 'INFANTRY' && dialog.armor_name.text == 'SYNTHMETAL' && dialog.armor_stat.text == 'Level 2');
dialog.submit(false); dialog.submit(false);
test.assert(dialog.pending && #sizeof(commands) == 1 && commands[0].name == 'commit_smac_workshop'
    && commands[0].data.base == base && commands[0].data.selected_design == 2 && commands[0].data.armor == 1 && commands[0].data.name == ''
    && commands[0].data.expected_state == 'before' && !commands[0].data.done);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 127, stale: false, failed: false, intrinsic_name_warning: false, done: false});
test.assert(dialog.active && !dialog.pending && dialog.selected_design == 127);
dialog.hide_view(); test.assert(!dialog.active && windows[0].hidden && windows[1].hidden && #sizeof(commands) == 1);
dialog.open(base, selection); dialog.submit(true);
listeners.smac_workshop_commit({base: {id: 702}}); test.assert(dialog.pending);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 127, stale: true}); test.assert(dialog.active && !dialog.pending && dialog.selected_design == 2);
dialog.submit(true); listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 127, stale: false, failed: true}); test.assert(dialog.active && !dialog.pending && dialog.selected_design == 2);
dialog.submit(true); listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 127, stale: false, failed: false, intrinsic_name_warning: true, done: true});
test.assert(!dialog.active && windows[0].hidden && dialog.draft_name == '');
dialog.open(base, selection); dialog.open_picker();
test.assert(windows[1].handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true && windows[1].hidden && dialog.active);
test.assert(windows[0].handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true && !dialog.active);
dialog.open(base, selection); listeners.turn_status({status: 'please_wait'}); test.assert(!dialog.active && windows[0].hidden);

// An older Done response must not close a newer session on the same Base.
dialog.open(base, selection); dialog.submit(true); const old_request = dialog.pending_id; dialog.hide_view();
dialog.open(base, selection); dialog.submit(false); const current_request = dialog.pending_id;
listeners.smac_workshop_commit({base: base, request_id: old_request, selection: selection, design_id: 127, stale: false, failed: false, intrinsic_name_warning: false, done: true});
test.assert(dialog.active && dialog.pending && current_request != old_request);
listeners.smac_workshop_commit({base: base, request_id: current_request, selection: selection, design_id: 127, stale: false, failed: false, intrinsic_name_warning: false, done: false});
test.assert(dialog.active && !dialog.pending && dialog.selected_design == 127);

const before_keys = #sizeof(commands);
test.assert(windows[0].handlers.keydown({modifiers: {shift: true}, code: 'ENTER'}) == true && #sizeof(commands) == before_keys);
test.assert(windows[0].handlers.keydown({modifiers: {}, code: 'ENTER'}) == true && dialog.pending && #sizeof(commands) == before_keys + 1 && commands[before_keys].data.done);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 127, stale: false, failed: false, intrinsic_name_warning: false, done: true});
test.assert(!dialog.active && windows[0].hidden);
