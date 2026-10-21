const dialog = #include('../default/ui/parts/game/popup/base_screen/workshop');
let windows = []; let commands = []; let listeners = {}; let edits = [];
const node = (data) => {
    data.hidden = false; data.handlers = {}; data.children = [];
    data.hide = () => { data.hidden = true; }; data.show = () => { data.hidden = false; }; data.remove = () => {};
    data.smacworkshoparmorpicker = (selected, rows) => { data.picker_selected = selected; data.picker_rows = rows; return {domains: ['Land', 'Sea', 'Air'], power: 'Power', shielding: 'Shielding', psi: 'Psi'}; };
    data.smacworkshopcomponentpicker = (kind, selected, rows, chassis, weapon, reactor, faction, power, mode, strength) => { data.picker_kind = kind; data.picker_selected = selected; data.picker_rows = rows; return {domains: ['Land', 'Sea', 'Air'], power: 'Power', shielding: 'Shielding', psi: 'Psi'}; };
    data.measure = () => { return {width: #sizeof(#split(data.text, ' ')) * 40}; };
    data.smacworkshopcomponents = (armor, only_armor) => { test.assert(only_armor); data.components_armor = armor; };
    data.smacworkshopstrip = (rows, faction, selected, hovered) => { data.strip_rows = rows; data.strip_faction = faction; data.strip_selected = selected; data.strip_hovered = hovered;
        return {normal: 'rgb(1,2,3)', unused: 'rgb(4,5,6)', obsolete: 'rgb(7,8,9)', empty_label: 'Empty Slot', obsolete_label: 'Obsolete'}; };
    data.smacworkshopdesign = (chassis, weapon, armor, reactor, kind, faction, yaw, power, mode, strength) => { data.preview_chassis = chassis; data.preview_weapon = weapon; data.preview_reactor = reactor; data.preview_strength = strength; data.preview_power = power; data.preview_mode = mode; data.preview_armor = armor; data.preview_yaw = yaw; data.preview_faction = faction; };
    data.on = (event, cb) => { data.handlers[event] = cb; };
    data.listen = (target, event, cb) => { listeners[event] = cb; };
    const child = (properties) => { const value = node(properties); data.children :+ value; return value; };
    data.surface = child; data.button = child; data.input = child; data.panel = child;
    if (!#is_defined(data.text)) { data.text = (properties) => { test.assert(!#is_defined(properties.width)); return child(properties); }; }
    return data;
};
dialog.init({ui: {class: (name) => { return {set: (properties) => { test.assert(#is_defined(properties._hover) && #is_defined(properties._active)); }}; }, root: {window: (data) => { const result = node(data); windows :+ result; return result; }}},
    game: {event: (name, data) => { commands :+ {name: name, data: data}; }}});
const base = {id: 701};
const selection = {expected_state: 'before', chassis: 'Infantry', weapon: 'Hand Weapons', reactor: 'Fission', scout_name: 'Scout', faction_art: 'morgan.pcx', speed: 1, offense: 1, weapon_mode: 0, power: 1,
    ability_names: ['Super Former', 'Deep Radar', '', '', '', '', '', '', '', 'Polymorphic', '', '', '', '', '', '', '', '', 'Trance', '', '', '', '', ''],
    chassis_choices: [{id: 0, name: 'Infantry', domain: 0, value: 1, technology_available: true}, {id: 3, name: 'Foil', domain: 1, value: 4, technology_available: false}],
    reactor_choices: [{id: 1, name: 'Fission', value: 1, technology_available: true}, {id: 2, name: 'Fusion', value: 2, technology_available: true}],
    armor: [{id: 0, name: 'No Armor', short_name: 'None', rating: 1, stat_line: '1-1-1 ', design_name: 'Scout', cost: 10, technology_available: true, available: true}, {id: 1, name: 'Synthmetal', short_name: 'Synthmetal', rating: 2, stat_line: '1-2-1 ', design_name: 'Synthmetal Garrison', cost: 20, technology_available: true, available: true}, {id: 2, name: 'Unknown', short_name: 'Unknown', rating: 3, stat_line: '1-3-1 ', design_name: '', cost: 0, technology_available: false, available: false}]};
base.get_smac_workshop_edit = (selected, chassis, weapon, armor, reactor, abilities, name, first, second, kind, choice, automatic) => {
    edits :+ {kind: kind, choice: choice, automatic: automatic, first: first, second: second};
    if (choice >= 0) { if (kind == 0) { chassis = choice; } if (kind == 2) { armor = choice; } if (kind == 3) { reactor = choice; } }
    const result = dialog.selection + {};
    result.draft = {selected_design: selected, chassis: chassis, weapon: weapon, armor: armor, reactor: reactor, abilities: abilities, name: name};
    result.ability_slots = [first, second]; return result;
};
test.assert(windows[0].left == 216 && windows[0].top == 168 && windows[0].width == 623 && windows[0].height == 387 && windows[0].children[0].background == 'deswin.pcx:smacsoftware()');
test.assert(dialog.component_models[0].model.top == (0 - 88) && dialog.component_models[1].model.top == (0 - 86) && dialog.component_models[2].model.top == (0 - 90));
test.assert(dialog.stats.baseline == 261);
let stat_geometry = false; for (child of windows[0].children) { if (child.left == 140 && child.top == 9 && child.height == 311) { stat_geometry = true; } }
test.assert(stat_geometry);
dialog.open(base, selection); test.assert(dialog.active && !windows[0].hidden && dialog.selected_armor == 0);
test.assert(dialog.model.preview_armor == 0 && dialog.model.preview_yaw == 0.0 && dialog.model.preview_faction == 'morgan.pcx');
dialog.open_picker(); test.assert(!windows[1].hidden && #sizeof(dialog.cards) == 3 && dialog.picker_canvas.picker_selected == 0 && #sizeof(dialog.picker_canvas.picker_rows) == 3 && #sizeof(dialog.cards[2].children) == 0);
test.assert(dialog.cards[2].handlers.click({}) == true && dialog.selected_armor == 0 && !windows[1].hidden);
test.assert(dialog.cards[1].handlers.click({}) == true && dialog.selected_armor == 1 && windows[1].hidden);
test.assert(dialog.model.preview_armor == 1 && dialog.model.preview_yaw == 0.0 && dialog.components.components_armor == 1);
test.assert(dialog.title.text == 'SYNTHMETAL GARRISON' && dialog.stats.text == '1-2-1 ' && dialog.cost.text == 'COST:  20' && dialog.chassis.text == 'INFANTRY' && dialog.armor_name.text == 'SYNTHMETAL' && dialog.armor_stat.text == 'Level 2');
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

// Overwrite answers are explicit, fenced, single-use and keep the original Done/Apply intent.
dialog.open(base, selection); dialog.selected_design = 64; dialog.selected_armor = 1; dialog.submit(true);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: false, confirmation: true, overwrite_name: 'Old Guard', overwrite_count: 3});
test.assert(dialog.active && dialog.confirming && !dialog.pending && !windows[2].hidden);
test.assert(#sizeof(dialog.overwrite_lines) >= 3 && #sizeof(dialog.overwrite_lines) <= 5);
for (line of dialog.overwrite_lines) { test.assert(line.measure().width <= 410); }
const before_modal = #sizeof(commands); dialog.submit(false); dialog.open_picker(); test.assert(#sizeof(commands) == before_modal && windows[1].hidden);
test.assert(windows[2].handlers.keydown({modifiers: {}, code: 'ENTER'}) == true && dialog.pending && !dialog.confirming && windows[2].hidden);
test.assert(commands[before_modal].data.overwrite == 0 && commands[before_modal].data.done && commands[before_modal].data.expected_state == 'before');
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: false, declined: true, restore_draft: false, done: false});
test.assert(dialog.active && !dialog.pending && dialog.selected_armor == 1);
dialog.submit(false);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: false, confirmation: true, overwrite_name: 'Old Guard', overwrite_count: 3});
dialog.answer_overwrite(false);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: false, declined: true, restore_draft: true, restored_armor: 0, restored_name: '', done: false});
test.assert(dialog.active && dialog.selected_armor == 0 && dialog.selected_design == 64);
dialog.selected_armor = 1; dialog.submit(true);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: false, confirmation: true, overwrite_name: 'Old Guard', overwrite_count: 3});
const before_accept = #sizeof(commands); dialog.answer_overwrite(true); dialog.answer_overwrite(true);
test.assert(#sizeof(commands) == before_accept + 1 && commands[before_accept].data.overwrite == 1 && commands[before_accept].data.done);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: false, intrinsic_name_warning: false, done: true});
test.assert(!dialog.active && windows[2].hidden);
dialog.open(base, selection); dialog.submit(false);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: false, confirmation: true, overwrite_name: 'Old Guard', overwrite_count: 3});
listeners.turn_status({status: 'please_wait'}); const after_close = #sizeof(commands); dialog.answer_overwrite(true);
test.assert(!dialog.active && windows[2].hidden && #sizeof(commands) == after_close);

// Researched advanced/PSI shielding keeps source identities through preview and Apply.
const advanced = selection + {unused: true};
advanced.armor = [{id: 0, name: 'No Armor', short_name: 'None', rating: 1, stat_line: '1-1-1 ', design_name: 'Scout', cost: 10, technology_available: true, available: true},
    {id: 9, name: 'Psi Defense', short_name: 'PSI', rating: (0 - 1), stat_line: '1-?-1 ', design_name: 'Psi Infantry', cost: 60, technology_available: true, available: true}];
dialog.open(base, advanced); dialog.open_picker(); test.assert(#sizeof(dialog.cards) == 2);
dialog.cards[1].handlers.click({});
test.assert(dialog.selected_armor == 9 && dialog.model.preview_armor == 9 && dialog.model.preview_strength == (0 - 1)
    && dialog.model.preview_power == 1 && dialog.model.preview_mode == 0 && dialog.components.components_armor == 9);
dialog.submit(false); test.assert(commands[#sizeof(commands) - 1].data.armor == 9);
dialog.hide_view();

// Component-strength captions use Psi; the full stat line uses a question mark.
for (row of [{value: (0 - 128), text: 'Psi'}, {value: (0 - 2), text: 'Psi'}, {value: (0 - 1), text: 'Psi'}, {value: 0, text: '0'}, {value: 12, text: '12'}]) {
    test.assert(dialog.armor_strength(row.value) == row.text);
}
dialog.open(base, advanced); dialog.open_picker();
let psi_number = false;
for (child of dialog.cards[1].children) { if (child.text == 'Psi') { psi_number = true; } }
test.assert(psi_number);
dialog.cards[1].handlers.click({});
test.assert(dialog.armor_stat.text == 'Level Psi' && dialog.stats.text == '1-?-1 ');
dialog.hide_view();

// Apply reselection is consumed before displaying a failed-edit message; Done keeps its draft.
dialog.open(base, selection); dialog.selected_design = 64; dialog.selected_armor = 0; dialog.draft_name = 'Unsaved'; dialog.submit(false);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: true, done: false,
    restore_draft: true, restored_armor: 1, restored_name: 'Retained Guard'});
test.assert(dialog.active && !dialog.pending && dialog.selected_design == 64 && dialog.selected_armor == 1 && dialog.draft_name == 'Retained Guard');
dialog.selected_armor = 0; dialog.draft_name = 'Edited'; dialog.submit(true);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selection, design_id: 64, stale: false, failed: true, done: false, restore_draft: false});
test.assert(dialog.active && dialog.selected_armor == 0 && dialog.draft_name == 'Edited'); dialog.hide_view();

// The complete live protocol preserves non-Scout chassis/weapon/reactor/abilities.
const full = selection + {draft: {selected_design: 64, chassis: 1, weapon: 1, armor: 1, reactor: 2, abilities: 512, name: 'Saved Rover'}};
full.chassis = 'Speeder'; full.weapon = 'Laser'; full.reactor = 'Fusion'; full.power = 2; full.offense = 2; full.speed = 2;
dialog.open(base, full);
test.assert(dialog.selected_design == 64 && dialog.selected_chassis == 1 && dialog.selected_weapon == 1 && dialog.selected_reactor == 2 && dialog.selected_abilities == 512 && dialog.draft_name == 'Saved Rover');
test.assert(dialog.model.preview_chassis == 1 && dialog.model.preview_weapon == 1 && dialog.model.preview_reactor == 2);
dialog.submit(false); const full_command = commands[#sizeof(commands) - 1].data;
test.assert(full_command.chassis == 1 && full_command.weapon == 1 && full_command.reactor == 2 && full_command.abilities == 512 && full_command.armor == 1);
const restored_full = full + {unused: true};
restored_full.draft = {selected_design: 65, chassis: 2, weapon: 2, armor: 0, reactor: 3, abilities: 262144, name: 'Retained Hover'};
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: restored_full, design_id: 65, stale: false, failed: true, done: false,
    restore_draft: true, restored_armor: 0, restored_name: 'Retained Hover'});
test.assert(dialog.selected_design == 65 && dialog.selected_chassis == 2 && dialog.selected_weapon == 2 && dialog.selected_reactor == 3 && dialog.selected_abilities == 262144 && dialog.draft_name == 'Retained Hover');
test.assert(dialog.model.preview_chassis == 2 && dialog.model.preview_weapon == 2 && dialog.model.preview_reactor == 3);
dialog.hide_view();

// Saved cards reserve ten native columns and navigate one cyclic browse pass.
let saved_rows = []; let saved_index = 0;
while (saved_index < 14) {
    saved_rows :+ {id: 64 + saved_index, active: saved_index != 1, excluded: false, flags: 1, stored_name: 'Retained Rover', unit_count: 0,
        components: {chassis: 1, weapon: 0, armor: 0, reactor: 1, offense: 1, weapon_mode: 0, strength: 1, speed: 2, stat_line: '1-1-2'}};
    saved_index++;
}
const browser = selection + {designs: saved_rows, draft: {selected_design: 64, chassis: 1, weapon: 0, armor: 0, reactor: 1, abilities: 0, name: ''}};
dialog.open(base, browser);
test.assert(dialog.strip_offset == 9 && dialog.strip_canvas.strip_rows[5].id == 64);
dialog.strip_offset = 0; dialog.render_strip();
test.assert(!dialog.strip.hidden && dialog.strip.left == 8 && dialog.strip.top == 703 && dialog.strip.width == 756 && dialog.strip.height == 56);
test.assert(#sizeof(dialog.strip_cards) == 10 && dialog.strip_cards[0].left == 19 && dialog.strip_cards[9].left == 667 && dialog.strip_cards[0].width == 70);
test.assert(dialog.strip_canvas.strip_rows[0].id == 64 && dialog.strip_canvas.strip_rows[9].id == 73);
dialog.scroll_strip(false); test.assert(dialog.strip_offset == 14 && dialog.strip_canvas.strip_rows[0].id == 64);
dialog.scroll_strip(true); test.assert(dialog.strip_offset == 15 && dialog.strip_canvas.strip_rows[0].id == 65);
dialog.scroll_strip(false); dialog.scroll_strip(false); test.assert(dialog.strip_offset == 13 && dialog.strip_canvas.strip_rows[0].id == 77 && dialog.strip_canvas.strip_rows[1].id == 64);
const before_select = #sizeof(commands); test.assert(dialog.strip_cards[0].handlers.click({}) == true && dialog.pending);
test.assert(commands[before_select].name == 'select_smac_workshop' && commands[before_select].data.selected_design == 77 && commands[before_select].data.expected_state == 'before');
dialog.scroll_strip(true); dialog.strip_cards[1].handlers.click({}); test.assert(dialog.strip_offset == 13 && #sizeof(commands) == before_select + 1);
const selected_browser = browser + {unused: true}; selected_browser.draft = {selected_design: 77, chassis: 1, weapon: 0, armor: 0, reactor: 1, abilities: 0, name: 'Retained Rover'};
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: selected_browser, design_id: 77, stale: false, failed: false, restore_draft: true, restored_armor: 0, restored_name: 'Retained Rover', intrinsic_name_warning: false, done: false});
test.assert(!dialog.pending && dialog.active && dialog.selected_design == 77 && dialog.selected_chassis == 1 && dialog.draft_name == 'Retained Rover');
dialog.hide_view(); test.assert(dialog.strip.hidden);
const short_browser = selection + {designs: [saved_rows[0] + {}, saved_rows[1] + {}]}; dialog.open(base, short_browser);
test.assert(#sizeof(dialog.strip_cards) == 2 && #sizeof(dialog.strip_canvas.strip_rows) == 2 && dialog.strip_cards[1].left == 91);
dialog.hide_view();

// Live control routing carries the complete tuple and explicit preference.
dialog.open(base, selection);
dialog.chassis_button.handlers.click({});
test.assert(dialog.picker_canvas.picker_kind == 0 && #sizeof(dialog.cards) == 2);
const before_edits = #sizeof(edits);
dialog.cards[1].handlers.click({}); test.assert(#sizeof(edits) == before_edits && !dialog.picker.hidden);
dialog.cards[0].handlers.click({}); test.assert(edits[#sizeof(edits) - 1].kind == 0 && dialog.picker.hidden);
dialog.reactor_button.handlers.click({});
test.assert(dialog.picker_canvas.picker_kind == 3 && dialog.picker_canvas.picker_selected == 1 && #sizeof(dialog.cards) == 2);
dialog.cards[1].handlers.click({}); test.assert(dialog.selected_reactor == 2 && edits[#sizeof(edits) - 1].choice == 2);
dialog.reactor_button.handlers.click({});
test.assert(dialog.picker.handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true);
test.assert(edits[#sizeof(edits) - 1].kind == 3 && edits[#sizeof(edits) - 1].choice == (0 - 1) && edits[#sizeof(edits) - 1].automatic && dialog.selected_reactor == 2 && dialog.picker.hidden);
dialog.automatic_suggestions = false; dialog.open_picker(); dialog.cards[1].handlers.click({});
test.assert(!edits[#sizeof(edits) - 1].automatic && edits[#sizeof(edits) - 1].kind == 2 && dialog.selected_armor == 1);
dialog.hide_view();