const dialog = #include('../default/ui/parts/game/popup/base_screen/workshop');
let windows = []; let commands = []; let listeners = {}; let edits = [];
const node = (data) => {
    data.hidden = false; data.removed = false; data.handlers = {}; data.children = [];
    data.hide = () => { data.hidden = true; }; data.show = () => { data.hidden = false; }; data.remove = () => { data.removed = true; };
    data.smacworkshoparmorpicker = (selected, rows) => { data.picker_selected = selected; data.picker_rows = rows; return {domains: ['Land', 'Sea', 'Air'], power: 'Power', shielding: 'Shielding', psi: 'Psi'}; };
    data.smacworkshopcomponentpicker = (kind, selected, rows, chassis, weapon, reactor, faction, power, mode, strength) => { data.picker_kind = kind; data.picker_selected = selected; data.picker_rows = rows; return {domains: ['Land', 'Sea', 'Air'], power: 'Power', shielding: 'Shielding', psi: 'Psi', weapon: 'Weapon', equipment: 'Equipment'}; };
    data.smacworkshopparenttext = () => { let colors = []; let index = 0; while (index < 256) { colors :+ ('color' + #to_string(index)); index++; }
        return {labels: ['Vehicle Design','Moves:','Type:','Power:','Prototype Cost:','Cost:','Data','Presets','W:','A:','C:','R:','SP1:','SP2:','E:','P:'],
            minimum: 'Min Cost', psi: 'Psi', colors: colors, weapon: 'Weapon', equipment: 'Equipment', obsolete: 'Obsolete', rename: 'Rename', retire: 'Retire', upgrade: 'Upgrade', status_title: 'Design Workshop', slot: 'Slot',
            status_labels: ['Examining Basic Unit','New Design Based On Old','Examining Previous Design','Designing New Unit','Modifying Previous Design']}; };
    data.smacworkshopconsolebacking = () => { data.console_tiled = true; };
    data.smacworkshoprenametext = () => { const words = data.smacworkshopparenttext(); return {title: 'What shall we call this unit type?', label: 'Name:', ok: 'OK', cancel: 'Cancel', colors: words.colors}; };
    data.smacworkshopretiretext = (name, count) => { const words = data.smacworkshopparenttext();
        return {title: 'Chief Engineer', body: count == 0 ? 'Do you wish to retire this design?' : 'Do you wish to retire this design? Units of that type still in operation will be disbanded.', rows: ['Never mind.', 'Retire ' + name + ' design.'], ok: 'OK', colors: words.colors}; };
    data.smacworkshopupgradetext = (kind, source, target, count, total, available) => {
        const words = data.smacworkshopparenttext();
        const body = kind == 5 ? 'Upgrade complete!' : (kind == 0 ? 'Upgrade this to which design:' : 'Upgrade this design at a cost?');
        let tokens = []; const parts = #split(body, ' '); let at = 0;
        for (part of parts) { const text = part + (at + 1 < #sizeof(parts) ? ' ' : ''); tokens :+ {text: text, spans: [{text: text, color: 226}]}; at++; }
        return {title: 'Operations Director', body: body, tokens: tokens, rows: kind == 1 || kind == 2 ? ['No, we cannot afford that.', 'Yes, modernize.'] : [], ok: 'OK', cancel: 'Cancel', colors: words.colors};
    };
    data.focus = () => { data.focused = true; }; data.select_all = () => { data.all_selected = true; };
    data.smacworkshopabilitytext = (technology) => { return {title: 'Select Special Ability', none: 'No special ability', ok: 'OK', cancel: 'Cancel', cost: 'Cost', attack: 'Attack', defense: 'Defense', speed: 'Speed', domains: ['Land', 'Sea', 'Air'], second_title: 'Chief Engineer', second_body: 'Research ' + technology + '.', missile_title: 'Chief Engineer', missile_body: 'Missiles cannot have special abilities.'}; };
    data.measure = () => { return {width: #sizeof(#split(data.text, ' ')) * 40}; };
    data.smacworkshopcomponents = (armor, only_armor) => { test.assert(only_armor); data.components_armor = armor; };
    data.smacworkshopstrip = (rows, faction, selected, hovered) => { data.strip_rows = rows; data.strip_faction = faction; data.strip_selected = selected; data.strip_hovered = hovered;
        return {normal: 'rgb(1,2,3)', unused: 'rgb(4,5,6)', obsolete: 'rgb(7,8,9)', empty_label: 'Empty Slot', obsolete_label: 'Obsolete'}; };
    data.smacworkshopdesign = (chassis, weapon, armor, reactor, kind, faction, yaw, power, mode, strength) => { data.preview_chassis = chassis; data.preview_weapon = weapon; data.preview_reactor = reactor; data.preview_strength = strength; data.preview_power = power; data.preview_mode = mode; data.preview_armor = armor; data.preview_yaw = yaw; data.preview_faction = faction; };
    data.on = (event, cb) => { data.handlers[event] = cb; };
    data.listen = (target, event, cb) => { listeners[event] = cb; };
    const child = (properties) => { const value = node(properties); data.children :+ value; return value; };
    data.surface = child; data.button = child; data.input = child; data.panel = child; data.scrollbar = child;
    if (!#is_defined(data.text)) { data.text = (properties) => { test.assert(!#is_defined(properties.width)); return child(properties); }; }
    return data;
};
dialog.init({ui: {class: (name) => { return {set: (properties) => { test.assert(#is_defined(properties._hover) && #is_defined(properties._active)); }}; }, root: {window: (data) => { const result = node(data); windows :+ result; return result; }}},
    game: {event: (name, data) => { commands :+ {name: name, data: data}; }}});
const base = {id: 701};
const release_card = (index) => { return dialog.picker_canvas.handlers.mouseup({button: 'left', x: 79 + (index % 5) * 108, y: 49 + #floor(#to_float(index) / 5.0) * 94}); };
const selection = {expected_state: 'before', mineral_factor: 10, weapon_cost: 1, movement: 1, chassis: 'Infantry', weapon: 'Hand Weapons', reactor: 'Fission', scout_name: 'Scout', faction_art: 'morgan.pcx', speed: 1, offense: 1, weapon_mode: 0, power: 1,
    ability_names: ['Super Former', 'Deep Radar', '', '', '', '', '', '', '', 'Polymorphic', '', '', '', '', '', '', '', '', 'Trance', '', '', '', '', ''],
    chassis_choices: [{id: 0, name: 'Infantry', domain: 0, value: 1, technology_available: true}, {id: 3, name: 'Foil', domain: 1, value: 4, technology_available: false}],
    reactor_choices: [{id: 1, name: 'Fission', value: 1, technology_available: true}, {id: 2, name: 'Fusion', value: 2, technology_available: true}],
    armor: [{id: 0, name: 'No Armor', short_name: 'None', rating: 1, armor_class: 0, stat_line: '1-1-1 ', design_name: 'Scout', cost: 10, cost_rows: 1, fast_land_warning: false, technology_available: true, available: true}, {id: 1, name: 'Synthmetal', short_name: 'Synthmetal', rating: 2, armor_class: 0, stat_line: '1-2-1 ', design_name: 'Synthmetal Garrison', cost: 20, cost_rows: 2, fast_land_warning: false, technology_available: true, available: true}, {id: 2, name: 'Unknown', short_name: 'Unknown', rating: 3, armor_class: 0, stat_line: '1-3-1 ', design_name: '', cost: 0, cost_rows: 0, fast_land_warning: false, technology_available: false, available: false}]};
let ability_menu = {guard: '', required_technology: 'Neural', rows: [{id: 24, name: '', description: '', cost: undefined}]};
base.get_smac_workshop_abilities = (selected, chassis, weapon, armor, reactor, abilities, name, first, second, slot) => { return ability_menu; };
base.get_smac_workshop_edit = (selected, chassis, weapon, armor, reactor, abilities, name, first, second, kind, choice, automatic) => {
    edits :+ {kind: kind, choice: choice, automatic: automatic, first: first, second: second};
    if (choice >= 0) { if (kind == 0) { chassis = choice; } if (kind == 1) { weapon = choice; } if (kind == 2) { armor = choice; } if (kind == 3) { reactor = choice; } }
    if (kind >= 4) {
        if (kind == 4) { first = choice >= 24 ? (0 - 1) : choice; } else { second = choice >= 24 ? (0 - 1) : choice; }
        abilities = 0; let bit = 0; let value = 1;
        while (bit < 24) { if (bit == first || bit == second) { abilities += value; } value *= 2; bit++; }
    }
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
test.assert(release_card(2) == true && dialog.selected_armor == 0 && windows[1].hidden);
dialog.open_picker();
test.assert(release_card(1) == true && dialog.selected_armor == 1 && windows[1].hidden);
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
advanced.armor = [{id: 0, name: 'No Armor', short_name: 'None', rating: 1, armor_class: 0, stat_line: '1-1-1 ', design_name: 'Scout', cost: 10, cost_rows: 1, fast_land_warning: false, technology_available: true, available: true},
    {id: 9, name: 'Psi Defense', short_name: 'PSI', rating: (0 - 1), armor_class: 2, stat_line: '1-?-1 ', design_name: 'Psi Infantry', cost: 60, cost_rows: 6, fast_land_warning: false, technology_available: true, available: true}];
dialog.open(base, advanced); dialog.open_picker(); test.assert(#sizeof(dialog.cards) == 2);
release_card(1);
test.assert(dialog.selected_armor == 9 && dialog.model.preview_armor == 9 && dialog.model.preview_strength == (0 - 1)
    && dialog.model.preview_power == 1 && dialog.model.preview_mode == 0 && dialog.components.components_armor == 9);
dialog.submit(false); test.assert(commands[#sizeof(commands) - 1].data.armor == 9);
dialog.hide_view();

// The native status projection changes only editor selection. It must reach
// the next Apply command, while a reply's older design_id must not undo it.
const status_view = selection + {status: 1};
status_view.draft = {selected_design: 67, chassis: 0, weapon: 0, armor: 0, reactor: 1, abilities: 0, name: ''};
dialog.open(base, status_view);
test.assert(dialog.selected_design == 67 && !dialog.console.hidden && dialog.console.zindex > dialog.window.zindex && dialog.console.zindex < dialog.strip.zindex);
test.assert(dialog.status_title.text == 'DESIGN WORKSHOP' && dialog.status_title.baseline == 18);
test.assert(dialog.status_lines[0].text == 'NEW DESIGN BASED ON OLD' && dialog.status_lines[0].baseline == 78);
dialog.submit(false); const status_request = commands[#sizeof(commands) - 1];
test.assert(status_request.data.selected_design == 67);
listeners.smac_workshop_commit({base: base, request_id: dialog.pending_id, selection: status_view, design_id: 64, stale: false, failed: false, intrinsic_name_warning: false, done: false});
test.assert(dialog.selected_design == 67);
status_view.status = 3; status_view.draft.selected_design = 127; dialog.selection = status_view; dialog.update_labels();
test.assert(dialog.status_lines[0].text == 'DESIGNING NEW UNIT (SLOT #64)');
const modifying = dialog.parent_text.status_labels[4];
dialog.parent_text.status_labels[4] = 'one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen';
status_view.status = 4; dialog.update_labels();
test.assert(#sizeof(dialog.status_lines) == 2 && dialog.status_lines[0].baseline == 56 && dialog.status_lines[1].baseline == 78);
dialog.parent_text.status_labels[4] = modifying;
dialog.hide_view(); test.assert(dialog.console.hidden);

// Component-strength captions use Psi; the full stat line uses a question mark.
for (row of [{value: (0 - 128), text: 'Psi'}, {value: (0 - 2), text: 'Psi'}, {value: (0 - 1), text: 'Psi'}, {value: 0, text: '0'}, {value: 12, text: '12'}]) {
    test.assert(dialog.armor_strength(row.value) == row.text);
}
dialog.open(base, advanced); dialog.open_picker();
let psi_number = false;
for (child of dialog.cards[1].children) { if (child.text == 'Psi') { psi_number = true; } }
test.assert(psi_number);
release_card(1);
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
release_card(1); test.assert(#sizeof(edits) == before_edits + 1 && edits[before_edits].choice == (0 - 1) && dialog.picker.hidden);
dialog.chassis_button.handlers.click({});
release_card(0); test.assert(edits[#sizeof(edits) - 1].kind == 0 && dialog.picker.hidden);
dialog.reactor_button.handlers.click({});
test.assert(dialog.picker_canvas.picker_kind == 3 && dialog.picker_canvas.picker_selected == 1 && #sizeof(dialog.cards) == 2);
release_card(1); test.assert(dialog.selected_reactor == 2 && edits[#sizeof(edits) - 1].choice == 2);
dialog.reactor_button.handlers.click({});
test.assert(dialog.picker.handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true);
test.assert(edits[#sizeof(edits) - 1].kind == 3 && edits[#sizeof(edits) - 1].choice == (0 - 1) && edits[#sizeof(edits) - 1].automatic && dialog.selected_reactor == 2 && dialog.picker.hidden);
dialog.automatic_suggestions = false; dialog.open_picker(); release_card(1);
test.assert(!edits[#sizeof(edits) - 1].automatic && edits[#sizeof(edits) - 1].kind == 2 && dialog.selected_armor == 1);
dialog.hide_view();
// Manual abilities are modal: selection/hover do not edit, None is always initial.
dialog.open(base, selection); dialog.ability_buttons[0].handlers.click({});
const ability = dialog.ability_dialog;
test.assert(ability.active && ability.selected == 0 && ability.rows[0].id == 24 && ability.window.height == 300 && ability.window.top == 255 && ability.list.height == 16 && ability.ok.top == 69 && ability.scroll.hidden);
const untouched = #sizeof(edits);
ability.window.handlers.keydown({code: 'ESCAPE', modifiers: {}}); test.assert(!ability.active && #sizeof(edits) == untouched);
let menu_rows = [];
for (id of [24,1,2,3,4,5,8,9,10,11,12,14,15,16,17,20,22]) {
    menu_rows :+ {id: id, name: 'Ability ' + #to_string(id), description: 'Description', cost: {quantity: 1, formula: 0, domain: 0}};
}
ability_menu = {guard: '', required_technology: 'Neural', rows: menu_rows};
dialog.ability_slots = [9,18]; dialog.selected_abilities = 262656;
dialog.ability_buttons[0].handlers.click({});
test.assert(ability.window.height == 345 && ability.window.top == 210 && ability.list.height == 225 && ability.ok.top == 278 && #sizeof(ability.buttons) == 12 && !ability.scroll.hidden && ability.selected == 0);
ability.buttons[1].handlers.mouseover({}); ability.buttons[1].handlers.click({}); ability.buttons[1].handlers.click({});
test.assert(ability.active && ability.selected == 1 && #sizeof(edits) == untouched && dialog.ability_slots == [9,18]);
ability.window.handlers.keydown({code: 'ESCAPE', modifiers: {}}); test.assert(#sizeof(edits) == untouched);
dialog.ability_buttons[0].handlers.click({}); test.assert(ability.selected == 0);
ability.window.handlers.keydown({code: 'DOWN', modifiers: {}}); ability.window.handlers.keydown({code: 'ENTER', modifiers: {}});
test.assert(!ability.active && edits[#sizeof(edits) - 1].kind == 4 && edits[#sizeof(edits) - 1].choice == 1 && dialog.ability_slots == [1,18]);
dialog.ability_buttons[0].handlers.click({}); ability.finish(true);
test.assert(dialog.ability_slots == [(0 - 1),18] && dialog.selected_abilities == 262144);
dialog.ability_buttons[0].handlers.click({}); ability.choose(16);
test.assert(ability.first == 5 && #sizeof(ability.buttons) == 12 && ability.buttons[11].smac_frame[0][0] == 29);
ability.buttons[11].handlers.mouseover({}); test.assert(ability.buttons[11].smac_frame[0][0] == 29);
ability.window.handlers.keydown({code: 'LEFT', modifiers: {}}); test.assert(ability.selected == 16);
ability.choose(0); ability.window.handlers.keydown({code: 'A', key: 'a', modifiers: {}}); test.assert(ability.selected == 1);
ability.window.handlers.keydown({code: 'TAB', modifiers: {}}); ability.window.handlers.keydown({code: 'TAB', modifiers: {}});
const before_tab = #sizeof(edits); ability.window.handlers.keydown({code: 'ENTER', modifiers: {}}); test.assert(!ability.active && #sizeof(edits) == before_tab);
ability_menu = {guard: '', required_technology: 'Neural', rows: [{id: 24, name: '', description: '', cost: undefined}, {id: 18, name: 'Trance', description: 'Defense', cost: {quantity: 0, formula: (0 - 1), domain: (0 - 1)}}]};
dialog.ability_slots = [(0 - 1),9]; dialog.selected_abilities = 512;
dialog.ability_buttons[0].handlers.click({}); ability.key({code: 'DOWN', modifiers: {}}); ability.finish(true);
test.assert(dialog.ability_slots == [18,9] && dialog.selected_abilities == 262656);
ability_menu = {guard: 'second_technology', required_technology: 'Neural', rows: []};
const before_guard = #sizeof(edits); dialog.ability_buttons[1].handlers.click({});
test.assert(ability.notice && ability.title.text == 'CHIEF ENGINEER' && ability.cancel.hidden && ability.lines[0].text == 'Research Neural.');
ability.finish(true); test.assert(!ability.active && #sizeof(edits) == before_guard);
dialog.hide_view();

// Weapon windows overlap by one row; unknown technology keeps its slot.
let weapons = []; let weapon_id = 0;
while (weapon_id < 21) {
    weapons :+ {id: weapon_id, name: 'Weapon ' + #to_string(weapon_id), value: weapon_id == 13 ? 99 : 10,
        mode: weapon_id < 14 ? 0 : 8, psi: weapon_id == 12, technology_available: weapon_id != 1};
    weapon_id++;
}
const weapon_view = selection + {weapon_choices: weapons};
dialog.open(base, weapon_view); dialog.selected_weapon = 20; dialog.weapon_button.handlers.click({});
test.assert(dialog.picker_canvas.picker_kind == 1 && dialog.picker_first == 5 && #sizeof(dialog.cards) == 16 && !dialog.picker_scroll.hidden && dialog.picker_scroll.max == 2.0);
test.assert(dialog.picker_canvas.picker_rows[0].id == 5 && dialog.picker_canvas.picker_rows[15].id == 20);
let equipment_label = false; for (child of dialog.cards[15].children) { if (child.text == 'Equipment') { equipment_label = true; } }
test.assert(equipment_label);
test.assert(dialog.picker_scroll.handlers.change({value: 2.0}) == true && dialog.picker_first == 10 && #sizeof(dialog.cards) == 11 && dialog.picker_canvas.picker_rows[10].id == 20);
release_card(10); test.assert(dialog.picker.hidden && edits[#sizeof(edits) - 1].kind == 1 && edits[#sizeof(edits) - 1].choice == (0 - 1));
// Paging retained A0 and same-ordinal release did not refresh it. A different
// hover establishes availability before returning to the payload card.
dialog.weapon_button.handlers.click({}); dialog.picker_scroll.handlers.change({value: 2.0});
dialog.picker_canvas.handlers.mousemove({x: 79, y: 49}); release_card(10);
test.assert(dialog.picker.hidden && edits[#sizeof(edits) - 1].choice == 20);
dialog.weapon_button.handlers.click({}); dialog.picker_scroll.handlers.change({value: 0.0});
const weapon_edits = #sizeof(edits); release_card(1);
test.assert(#sizeof(edits) == weapon_edits + 1 && edits[weapon_edits].choice == (0 - 1) && dialog.picker.hidden && #sizeof(dialog.cards[1].children) == 0);
dialog.weapon_button.handlers.click({}); dialog.picker_scroll.handlers.change({value: 0.0});
let psi_weapon = false; let buster_value = false;
for (child of dialog.cards[12].children) { if (child.text == 'Psi') { psi_weapon = true; } }
for (child of dialog.cards[13].children) { if (child.text == '99') { buster_value = true; } }
test.assert(psi_weapon && buster_value);
release_card(14); test.assert(dialog.selected_weapon == 14 && dialog.picker.hidden);
dialog.hide_view();
let stock_weapons = []; for (row of weapons) { if (row.id != 19) { stock_weapons :+ row + {}; } }
const stock_weapon_view = selection + {weapon_choices: stock_weapons};
dialog.open(base, stock_weapon_view); dialog.selected_weapon = 20; dialog.weapon_button.handlers.click({});
test.assert(dialog.picker_first == 0 && #sizeof(dialog.cards) == 20 && dialog.picker_canvas.picker_rows[19].id == 20 && dialog.picker_scroll.hidden);
dialog.picker.handlers.keydown({code: 'ESCAPE', modifiers: {}}); test.assert(dialog.picker.hidden && dialog.selected_weapon == 20);
// A filtered current ID scans past the entire admitted list, not just lower IDs.
dialog.selected_weapon = 19; dialog.weapon_button.handlers.click({});
test.assert(dialog.picker_first == 5 && #sizeof(dialog.cards) == 15 && dialog.picker_canvas.picker_selected == 19 && dialog.picker_canvas.picker_rows[14].id == 20);
dialog.hide_view();

// Live navigation highlights unknown cards, preserves text nodes and closes to
// the original value when the stored painter availability is zero.
dialog.open(base, selection); dialog.chassis_button.handlers.click({});
const existing_card = dialog.cards[0]; const navigation_edits = #sizeof(edits);
test.assert(dialog.picker_state.selected == 0 && dialog.picker_state.availability == 0);
dialog.picker.handlers.keydown({code: 'RIGHT', modifiers: {}});
test.assert(dialog.picker_state.selected == 1 && dialog.picker_state.availability == 0 && !existing_card.removed && #sizeof(edits) == navigation_edits && dialog.picker_canvas.picker_selected == 3);
dialog.picker.handlers.keydown({code: 'ENTER', modifiers: {}});
test.assert(dialog.picker.hidden && edits[#sizeof(edits) - 1].choice == (0 - 1) && dialog.selected_chassis == 0);
dialog.chassis_button.handlers.click({}); dialog.picker_canvas.handlers.mousemove({x: 4, y: 98});
test.assert(dialog.picker_state.selected == (0 - 1) && dialog.picker_canvas.picker_selected == (0 - 1));
test.assert(dialog.picker_canvas.handlers.mouseup({button: 'right', x: 79, y: 49}) == true && !dialog.picker.hidden);
dialog.picker_canvas.handlers.mouseup({button: 'left', x: 4, y: 98}); test.assert(dialog.picker.hidden);
dialog.hide_view();

// Window creation can deliver a pointer move at the opening button. Normalize
// an automated keyboard sequence with Home; never assume the cursor stayed0.
dialog.open(base, weapon_view); dialog.weapon_button.handlers.click({});
dialog.picker_canvas.handlers.mousemove({x: 79, y: 142}); test.assert(dialog.picker_state.selected == 5);
dialog.picker.handlers.keydown({code: 'HOME', modifiers: {}}); test.assert(dialog.picker_state.selected == 0);
for (code of ['DOWN','DOWN','RIGHT','RIGHT','RIGHT','RIGHT']) { dialog.picker.handlers.keydown({code: code, modifiers: {}}); }
test.assert(dialog.picker_state.selected == 14 && dialog.picker_state.availability == 1);
dialog.picker.handlers.keydown({code: 'ENTER', modifiers: {}}); test.assert(dialog.selected_weapon == 14 && dialog.picker.hidden); dialog.hide_view();

// Main COST uses the open-time factor and recomputed rows, not legacy quote
// totals. This minimum branch is a declared native presentation control.
const price_view = selection + {unused_price: true}; price_view.mineral_factor = 8;
price_view.armor = [selection.armor[0] + {}]; price_view.armor[0].cost_rows = 1; price_view.armor[0].cost = 999;
dialog.open(base, price_view); test.assert(dialog.cost.text == 'COST:  8');
dialog.selection.mineral_factor = 10; dialog.update_armor(); test.assert(dialog.cost.text == 'COST:  8');
dialog.selected_reactor = 2; dialog.update_armor(); test.assert(dialog.cost.text == 'COST:  (MIN COST: 16)');
price_view.armor[0].cost_rows = 6; price_view.armor[0].fast_land_warning = true; dialog.update_armor();
test.assert(dialog.cost.text == 'COST:  48!' && dialog.cost.color == 'color249');
dialog.selection.weapon_mode = 8; dialog.selection.weapon_cost = 10; dialog.update_labels();
test.assert(dialog.weapon_heading.text == 'EQUIPMENT' && dialog.weapon_stat.text == 'Equipment 10');
dialog.hide_view(); dialog.open(base, price_view); test.assert(dialog.mineral_factor == 10); dialog.hide_view();

dialog.open(base, selection);
test.assert(dialog.data_heading.text == 'DATA' && dialog.data_rows[0].text == 'W: 1' && dialog.data_rows[1].text == 'A: 1' && dialog.data_rows[2].text == 'C: 1' && dialog.data_rows[3].text == 'R: 1' && dialog.data_rows[4].text == 'SP1: -');
test.assert(dialog.data_rows[0].baseline == 122 && dialog.data_rows[5].baseline == 192);
dialog.ability_slots = [(0 - 1),1]; dialog.update_armor(); test.assert(dialog.data_rows[4].text == 'SP1: -' && dialog.data_rows[5].text == 'SP2:R');
dialog.hide_view();

// Obsolete publishes selected saved identity and the old list, without saving
// draft components or waiting for Apply. Duplicate clicks wait for the reply.
const obsolete_view = selection + {};
obsolete_view.designs = [{id: 2, active: true, excluded: false, flags: 1, stored_name: 'Scout', components: {chassis: 0, weapon: 0, armor: 0, reactor: 1, offense: 1, weapon_mode: 0, strength: 1, speed: 1, stat_line: '1-1-1'}},
    {id: 64, active: false, excluded: false, flags: 0, stored_name: '', components: undefined}];
dialog.open(base, obsolete_view); dialog.selected_armor = 1; dialog.draft_name = 'Unsaved Guard'; dialog.strip_offset = 1;
const obsolete_count = #sizeof(commands); dialog.obsolete(); dialog.obsolete();
test.assert(dialog.pending && dialog.pending_obsolete && #sizeof(commands) == obsolete_count + 1);
const obsolete_command = commands[obsolete_count];
test.assert(obsolete_command.name == 'obsolete_smac_workshop' && obsolete_command.data.old_list == [2,64]
    && obsolete_command.data.selected_design == 2 && obsolete_command.data.name == 'Unsaved Guard' && obsolete_command.data.strip_offset == 1);
const obsolete_reply = obsolete_view + {draft: {selected_design: 64, chassis: 0, weapon: 0, armor: 0, reactor: 1, abilities: 0, name: 'Unsaved Guard'}};
listeners.smac_workshop_obsolete({base: base, request_id: dialog.pending_id + 1}); test.assert(dialog.pending);
listeners.smac_workshop_obsolete({base: base, request_id: dialog.pending_id, selection: obsolete_reply, strip_offset: 1, stale: false, failed: false});
test.assert(!dialog.pending && dialog.active && dialog.selected_design == 64 && dialog.selected_armor == 0 && dialog.draft_name == 'Unsaved Guard' && dialog.strip_offset == 1);
dialog.hide_view(); test.assert(#sizeof(commands) == obsolete_count + 1);

// Rename open adopts the post-flag fence. Local Cancel sends no mutation and
// keeps that fence; accepted text updates the actor title without reselecting.
dialog.open(base, selection); dialog.selected_design = 64; const rename_count = #sizeof(commands);
dialog.rename_button.handlers.click(); dialog.rename_dialog.begin(dialog);
test.assert(dialog.pending && dialog.pending_rename && #sizeof(commands) == rename_count + 1 && commands[rename_count].name == 'open_smac_workshop_rename');
const rename_open_id = dialog.pending_id;
listeners.smac_workshop_rename({base: base, request_id: rename_open_id, answer: false, stale: false, failed: false, expected_state: 'after-open', default_name: 'Scout Patrol', name: '', display_name: 'Scout Patrol'});
const rename = dialog.rename_dialog;
test.assert(!dialog.pending && rename.active && rename.window.left == 148 && rename.window.top == 255 && rename.window.width == 760);
test.assert(rename.input.value == 'Scout Patrol' && rename.input.native_edit && rename.input.max_bytes == 31 && rename.input.focused && rename.input.all_selected);
rename.window.handlers.keydown({code: 'ESCAPE', modifiers: {}});
test.assert(!rename.active && dialog.active && #sizeof(commands) == rename_count + 1 && dialog.selection.expected_state == 'after-open');
rename.begin(dialog); listeners.smac_workshop_rename({base: base, request_id: dialog.pending_id, answer: false, stale: false, failed: false, expected_state: 'opened-again', default_name: 'Scout Patrol', name: '', display_name: 'Scout Patrol'});
const rename_selected = dialog.selected_design; const rename_armor = dialog.selected_armor;
rename.input.value = 'Draft Guard'; test.assert(rename.input.handlers.input({value: 'Draft Guard'}) == true);
test.assert(dialog.pending && !rename.active && commands[#sizeof(commands) - 1].name == 'answer_smac_workshop_rename'
    && commands[#sizeof(commands) - 1].data.submitted == 'Draft Guard' && commands[#sizeof(commands) - 1].data.displayed_default == 'Scout Patrol'
    && commands[#sizeof(commands) - 1].data.expected_state == 'opened-again');
listeners.smac_workshop_rename({base: base, request_id: dialog.pending_id, answer: true, stale: false, failed: false, expected_state: 'after-answer', default_name: 'Scout Patrol', name: 'Draft Guard', display_name: 'Draft Guard'});
test.assert(dialog.active && !dialog.pending && dialog.draft_name == 'Draft Guard' && dialog.title.text == 'DRAFT GUARD'
    && dialog.selected_design == rename_selected && dialog.selected_armor == rename_armor);
dialog.submit(false); test.assert(commands[#sizeof(commands) - 1].data.name == 'Draft Guard'); dialog.hide_view();

// Direct Retire uses the stored selected design, never the unsaved components.
dialog.open(base, selection); dialog.draft_name = 'Unsaved Draft'; const retire = dialog.retire_dialog;
retire.begin(dialog); const retire_commands = #sizeof(commands); retire.begin(dialog);
test.assert(dialog.pending && #sizeof(commands) == retire_commands && commands[retire_commands - 1].name == 'retire_smac_workshop'
    && commands[retire_commands - 1].data.confirm == (0 - 1) && commands[retire_commands - 1].data.name == 'Unsaved Draft'
    && !#is_defined(commands[retire_commands - 1].data.armor));
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: true, expected_state: 'retire-quote', count: 0, stored_name: 'Stored Scout', illustration: 'unit_sm.pcx'});
test.assert(retire.active && !dialog.pending && retire.window.left == 150 && retire.window.top == 255 && retire.window.width == 755 && retire.window.height == 300
    && retire.rows[0].button.top == 165 && retire.rows[1].button.top == 187 && retire.ok.top == 217 && retire.rows[1].label.text == 'Retire Stored Scout design.');
test.assert(retire.title.color == 'color226' && retire.body[0].color == 'color226' && retire.ok.text == 'OK' && retire.illustration.background == 'unit_sm.pcx:smacsoftware()');
test.assert(retire.rows[1].button.handlers.mouseover({}) == true && retire.selected == 0);
test.assert(retire.rows[1].label.color == 'color223');
test.assert(retire.rows[1].button.handlers.mousedown({button: 'left'}) == true && retire.selected == 1 && #sizeof(commands) == retire_commands);
test.assert(retire.rows[1].button.handlers.click({}) == true && retire.selected == 1 && #sizeof(commands) == retire_commands);
retire.window.handlers.keydown({modifiers: {}, code: 'HOME'});
test.assert(retire.window.handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true && dialog.pending && !retire.active && commands[#sizeof(commands) - 1].data.confirm == 0);
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: false, retired: false, declined: true, expected_state: 'retire-quote'});
test.assert(!dialog.pending && dialog.draft_name == 'Unsaved Draft' && dialog.selected_design == 2);
retire.begin(dialog);
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: true, expected_state: 'retire-live', count: 2, stored_name: 'Stored Scout', illustration: 'unit_sm.pcx'});
test.assert(retire.rows[0].button.top == 165 && retire.ok.top == 217 && #sizeof(retire.body) == 2);
retire.window.handlers.keydown({modifiers: {}, code: 'DOWN'}); retire.window.handlers.keydown({modifiers: {}, code: 'ESCAPE'});
test.assert(dialog.pending && commands[#sizeof(commands) - 1].data.confirm == 1 && commands[#sizeof(commands) - 1].data.expected_state == 'retire-live');
const after_retire = selection + {}; after_retire.expected_state = 'retired';
after_retire.draft = {selected_design: 64, chassis: 0, weapon: 0, armor: 0, reactor: 1, abilities: 0, name: 'Unsaved Draft'};
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: false, retired: true, declined: false, expected_state: 'retired', selection: after_retire, strip_offset: 7});
test.assert(!dialog.pending && !retire.active && dialog.active && dialog.selected_design == 64 && dialog.draft_name == 'Unsaved Draft' && dialog.strip_offset == 7);
retire.begin(dialog);
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: true, expected_state: 'retire-double', count: 0, stored_name: 'Stored Scout', illustration: 'unit_sm.pcx'});
test.assert(retire.rows[1].button.handlers.dblclick({button: 'left'}) == true && dialog.pending && commands[#sizeof(commands) - 1].data.confirm == 1);
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: false, retired: false, declined: true, expected_state: 'retire-double'});
retire.begin(dialog);
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: true, expected_state: 'retire-shift-enter', count: 0, stored_name: 'Stored Scout', illustration: 'unit_sm.pcx'});
test.assert(retire.window.handlers.keydown({modifiers: {shift: true}, code: 'ENTER'}) == true && dialog.pending && commands[#sizeof(commands) - 1].data.confirm == 0);
listeners.smac_workshop_retire({base: base, request_id: dialog.pending_id, stale: false, failed: false, confirmation: false, retired: false, declined: true, expected_state: 'retire-shift-enter'});
retire.begin(dialog); const retired_request = dialog.pending_id; dialog.hide_view();
listeners.smac_workshop_retire({base: base, request_id: retired_request, confirmation: true}); test.assert(!retire.active && !dialog.active);

// Upgrade queries/answers remain separate from editing or applying the draft.
dialog.open(base, selection); dialog.draft_name = 'Unsaved Draft'; const upgrade = dialog.upgrade_dialog;
upgrade.begin(dialog); const upgrade_commands = #sizeof(commands); upgrade.begin(dialog);
test.assert(dialog.pending && #sizeof(commands) == upgrade_commands && commands[upgrade_commands - 1].name == 'upgrade_smac_workshop'
    && commands[upgrade_commands - 1].data.target_design == (0 - 1) && !#is_defined(commands[upgrade_commands - 1].data.armor));
let upgrade_reply = {base: base, request_id: dialog.pending_id, expected_state: 'upgrade-quote', stale: false, failed: false, declined: false, upgraded: false,
    confirmation: false, affordable: true, unprototyped: false, count: 2, total: 100, available: 2000, target_design: (0 - 1),
    source_name: 'Stored Scout', source_text: {name: 'Stored Scout', stats: '1-1-1', domain: 0, reactor: 1, abilities: 0, ability_tokens: ''}, target_text: {name: 'Guard', stats: '2-3-1', domain: 0, reactor: 1, abilities: 0, ability_tokens: ''}, illustration: 'unit_sm.pcx',
    choices: [{id: 65, stored_name: 'Guard', stat_line: '2-3-1', prototype: true}, {id: 66, stored_name: 'Better Guard', stat_line: '4-3-1', prototype: false}]};
listeners.smac_workshop_upgrade(upgrade_reply);
test.assert(upgrade.rows[0].stats.text == '2-3-1 *' && upgrade.rows[1].stats.text == '4-3-1');
test.assert(upgrade.active && upgrade.kind == 0 && !dialog.pending && upgrade.window.width == 757 && upgrade.answers[0].left == 34 && upgrade.answers[1].left == 365);
test.assert(upgrade.rows[1].button.handlers.mousedown({button: 'left'}) == true && upgrade.selected == 1);
test.assert(upgrade.window.handlers.keydown({modifiers: {}, code: 'ESCAPE'}) == true && !upgrade.active && !dialog.pending && #sizeof(commands) == upgrade_commands);
upgrade.begin(dialog); upgrade_reply.request_id = dialog.pending_id; listeners.smac_workshop_upgrade(upgrade_reply);
upgrade.rows[1].button.handlers.dblclick({button: 'left'});
test.assert(dialog.pending && !upgrade.active && commands[#sizeof(commands) - 1].data.target_design == 66 && commands[#sizeof(commands) - 1].data.confirm == (0 - 1));
upgrade_reply.request_id = dialog.pending_id; upgrade_reply.target_design = 66; upgrade_reply.confirmation = true; listeners.smac_workshop_upgrade(upgrade_reply);
test.assert(upgrade.kind == 1 && upgrade.active && upgrade.window.width == 755 && upgrade.rows[0].button.top == 165 && upgrade.answers[0].top == 217 && #sizeof(upgrade.answers) == 1);
upgrade.window.handlers.keydown({modifiers: {}, code: 'DOWN'}); upgrade.window.handlers.keydown({modifiers: {}, code: 'ESCAPE'});
test.assert(dialog.pending && commands[#sizeof(commands) - 1].data.confirm == 1);
upgrade_reply.request_id = dialog.pending_id; upgrade_reply.upgraded = true; upgrade_reply.confirmation = false; upgrade_reply.zero_count_notice = false;
upgrade_reply.selection = after_retire; upgrade_reply.strip_offset = 3; listeners.smac_workshop_upgrade(upgrade_reply);
test.assert(!dialog.pending && !upgrade.active && dialog.strip_offset == 3 && dialog.active);
upgrade.begin(dialog); upgrade_reply.request_id = dialog.pending_id; upgrade_reply.upgraded = false; upgrade_reply.affordable = false; listeners.smac_workshop_upgrade(upgrade_reply);
test.assert(upgrade.active && upgrade.kind == 3 && upgrade.window.width == 758 && #sizeof(upgrade.rows) == 0 && upgrade.answers[0].top == 167);
const before_notice = #sizeof(commands); upgrade.window.handlers.keydown({modifiers: {}, code: 'ENTER'}); test.assert(!upgrade.active && #sizeof(commands) == before_notice);
upgrade.begin(dialog); upgrade_reply.request_id = dialog.pending_id; upgrade_reply.affordable = true; upgrade_reply.upgraded = true; upgrade_reply.zero_count_notice = true; listeners.smac_workshop_upgrade(upgrade_reply);
test.assert(upgrade.kind == 5 && upgrade.active && dialog.active); upgrade.window.handlers.keydown({modifiers: {shift: true}, code: 'ENTER'}); test.assert(!upgrade.active);
upgrade.begin(dialog); const upgrade_old = dialog.pending_id; dialog.hide_view(); listeners.smac_workshop_upgrade({base: base, request_id: upgrade_old, confirmation: true}); test.assert(!upgrade.active && !dialog.active);

// Native627740 uses a column-major grid, not a scrollbar. Its width measure
// includes the literal pipe; this double supplies the native measured174px.
dialog.open(base, selection); upgrade.begin(dialog); upgrade_reply.request_id = dialog.pending_id;
upgrade_reply.upgraded = false; upgrade_reply.zero_count_notice = false; upgrade_reply.target_design = (0 - 1); upgrade_reply.choices = [];
let ordinal = 0; while (ordinal < 13) { upgrade_reply.choices :+ {id: 65 + ordinal, stored_name: 'Upgrade Target ' + #to_string(65 + ordinal), stat_line: '2-3-1', prototype: false}; ordinal++; }
upgrade.probe.measure = () => { return {width: 174}; };
listeners.smac_workshop_upgrade(upgrade_reply);
test.assert(upgrade.rows_per_column == 7 && upgrade.row_width == 327 && upgrade.window.width == 757 && upgrade.window.height == 300
    && upgrade.answers[0].top == 227 && upgrade.answers[1].left == 365 && #sizeof(upgrade.rows) == 13);
let i = 0; for (row of upgrade.rows) { test.assert(row.button.left == 33 + (i / 7) * 330 && row.button.top == 65 + (i % 7) * 22); i++; }
upgrade.window.handlers.keydown({modifiers: {}, code: 'LEFT'}); test.assert(upgrade.selected == 7);
upgrade.window.handlers.keydown({modifiers: {}, code: 'RIGHT'}); test.assert(upgrade.selected == 0);
upgrade.rows[12].button.handlers.dblclick({button: 'left'}); test.assert(commands[#sizeof(commands) - 1].data.target_design == 77);
upgrade_reply.request_id = dialog.pending_id; upgrade_reply.choices = []; ordinal = 0;
while (ordinal < 63) { upgrade_reply.choices :+ {id: 65 + ordinal, stored_name: 'Upgrade Target ' + #to_string(65 + ordinal), stat_line: '2-3-1', prototype: false}; ordinal++; }
listeners.smac_workshop_upgrade(upgrade_reply);
test.assert(upgrade.rows_per_column == 11 && upgrade.row_width == 177 && upgrade.window.width == 1177 && upgrade.window.height == 382 && upgrade.window.left == (0 - 60) && upgrade.window.top == 173
    && upgrade.answers[0].top == 315 && upgrade.answers[0].width == 535 && upgrade.answers[1].left == 575 && #sizeof(upgrade.rows) == 63);
i = 0; for (row of upgrade.rows) { test.assert(row.button.left == 33 + (i / 11) * 180 && row.button.top == 65 + (i % 11) * 22); i++; }
upgrade.rows[62].button.handlers.dblclick({button: 'left'}); test.assert(commands[#sizeof(commands) - 1].data.target_design == 127);
dialog.hide_view();
