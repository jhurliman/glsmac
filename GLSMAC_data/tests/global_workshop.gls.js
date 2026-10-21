const controller = #include('../default/ui/parts/game/workshop/workshop');
let key = null; let calls = []; let closed = 0; let messages = []; let fail = false; let ruleset = 'smac';
const data = {preset_state: 'state'};
const player = {id: 0, get_smac_workshop: () => { if (fail) { throw Error('not ready'); } return data; }};
const editor = {active: false, opening: false, begin: (owner, selection, hint) => { calls :+ {owner: owner, selection: selection, hint: hint}; }, hide_view: () => { this.opening = false; }};
const popup = {popup: null, popup_defs: {base_screen: {sections: {workshop: editor}}}};
const p = {root: {on: (event, cb) => { test.assert(event == 'keydown'); key = cb; }},
    game: {get_ruleset: () => { return ruleset; }, get_player: () => { return player; }},
    modules: {popup: popup, menu: {close_all: () => { closed++; }}, first_base: {pending: null}, research: {pending: null}, colony: {pending: null}},
    process_message: (text) => { messages :+ text; }};
p.modules.popup = popup; popup.popup_defs.base_screen.sections.workshop = editor;
controller.init(p);
test.assert(!key({code: 'F4', modifiers: {}}));
test.assert(!key({code: 'U', modifiers: {ctrl: true}}) && !key({code: 'U', modifiers: {shift: true}}));
test.assert(key({code: 'U', modifiers: {}}) && #sizeof(calls) == 1 && closed == 1);
test.assert(calls[0].owner == player && calls[0].selection == data && calls[0].hint == (0 - 1));
for (name of ['first_base', 'research', 'colony']) {
    p.modules[name].pending = {}; test.assert(!controller.open() && #sizeof(calls) == 1); p.modules[name].pending = null;
}
popup.popup = {}; test.assert(!controller.open()); popup.popup = null;
editor.active = true; test.assert(!controller.open()); editor.active = false;
editor.opening = true; test.assert(!controller.open()); editor.opening = false;
ruleset = 'smax'; test.assert(!controller.open()); ruleset = 'smac';
fail = true; test.assert(controller.open() && #sizeof(calls) == 1 && #sizeof(messages) == 1 && !editor.opening); fail = false;
const hq = #include('../default/ui/parts/game/menu/left_menu_hq'); let entries = [];
hq.init({create: (rows) => { entries = rows; }, open_workshop: () => { controller.open(); }});
test.assert(#sizeof(entries) == 1); entries[0].open();
test.assert(#sizeof(calls) == 2 && calls[1].owner == player && calls[1].hint == (0 - 1));

const menu = #include('../default/ui/parts/game/menu/menu'); let registered = false;
for (name of menu.available_menus) { registered = registered || name == 'left_menu_hq'; } test.assert(registered);
const left = #include('../default/ui/parts/game/menu/left_menu'); let root_entries = []; let submenu = ''; let submenu_width = 0;
left.init({create: (rows) => { root_entries = rows; }, menu: {menu_width: 139, item_height: 17,
    open: (name, style) => { submenu = name; submenu_width = style.width; }, close: (name) => { test.assert(name == 'left_menu_hq'); }}});
test.assert(#sizeof(root_entries) == 2 && root_entries[1].label == 'HQ'); root_entries[1].open();
test.assert(submenu == 'left_menu_hq' && submenu_width == 230); root_entries[1].close();
