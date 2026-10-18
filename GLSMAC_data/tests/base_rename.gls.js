// Real optional Rename dialog binds narrow input and native issued authority.
const dialog = #include('../default/ui/parts/game/popup/base_screen/rename');
let commands = []; let actions = []; let listeners = {}; let keys = undefined;
let input_handlers = {};
const field = {value: '', on: (event, cb) => { input_handlers[event] = cb; }};
const window = {hidden: false, hide: () => { this.hidden = true; }, show: () => { this.hidden = false; },
    text: (data) => { return data; }, input: (data) => { return field; },
    button: (data) => { return {on: (event, cb) => { actions :+ cb; }}; },
    on: (event, cb) => { keys = cb; }, listen: (target, event, cb) => { listeners[event] = cb; }};
dialog.init({ui: {root: {window: (data) => { test.assert(data.zindex == 0.9); return window; }}}, game: {event: (name, data) => { commands :+ {name: name, data: data}; }}});
let state = 'prepared';
const base = {id: 701, get_smac_rename_state: () => { return state; }};
const request = {request_id: 7, expected_state: 'prepared', name: 'Duplicate', max_bytes: 24};
dialog.open(base, request);
test.assert(dialog.active && !window.hidden && field.value == 'Duplicate');
test.assert(input_handlers.change({value: '12345678901234567890123456789'}) == true);
test.assert(field.value == '123456789012345678901234');
field.value = ''; actions[0](); actions[0]();
test.assert(!dialog.active && window.hidden && #sizeof(commands) == 1 && commands[0].name == 'commit_smac_rename' && commands[0].data.name == ''
    && commands[0].data.base == base && commands[0].data.request_id == 7 && commands[0].data.expected_state == 'prepared');
dialog.open(base, request); field.value = 'scratch'; actions[1]();
test.assert(#sizeof(commands) == 2 && commands[1].name == 'cancel_smac_rename');
dialog.open(base, request); dialog.on_base_update(base); test.assert(dialog.active);
state = 'changed'; dialog.on_base_update(base); test.assert(!dialog.active && #sizeof(commands) == 2);
state = 'prepared'; dialog.open(base, request); listeners.research_choice({}); test.assert(!dialog.active);
dialog.open(base, request); keys({modifiers: {}, code: 'ESCAPE'}); test.assert(!dialog.active && #sizeof(commands) == 3);
const screen = #include('../default/ui/parts/game/popup/base_screen');
screen.base = base; screen.is_open = true; screen.production_selection = null;
screen.p = {game: {event: (name, data) => { commands :+ {name: name, data: data}; }}};
screen.open_rename(); test.assert(#sizeof(commands) == 4 && commands[3].name == 'open_smac_rename' && commands[3].data.base == base && commands[3].data.expected_state == 'prepared');
state = undefined; screen.open_rename(); test.assert(#sizeof(commands) == 4 && !screen.can_rename());

// Default buttons are initialized before a Base is bound and refreshed on set.
const buttons = #include('../default/ui/parts/game/popup/base_screen/buttons');
let available = false; let opened = 0;
const child = (data) => { data.remove = () => {}; data.on = (event, cb) => { data.click = cb; }; return data; };
const frame = {text: child, button: child};
frame.panel = (data) => { return frame; };
const style = {extend: (name) => { return this; }, set: (data) => {}};
buttons.init({body: frame, ui: {class: (name) => { return style; }}, hide: () => {}, utils: {
    can_rename: () => { return available; }, open_rename: () => { opened++; }}});
test.assert(buttons.buttons[0].text == 'Rename unavailable');
available = true; buttons.show_default(); buttons.buttons[0].click();
test.assert(buttons.buttons[0].text == 'RENAME' && opened == 1);
