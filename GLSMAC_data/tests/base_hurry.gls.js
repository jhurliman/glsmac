const dialog = #include('../default/ui/parts/game/popup/base_screen/hurry');
let commands = []; let actions = []; let listeners = {}; let keys = undefined; let handlers = {};
let focused = true;
const field = {value: '', hide: () => { test.assert(focused); focused = false; }, show: () => { test.assert(!focused); focused = true; }, on: (event, cb) => { handlers[event] = cb; }};
const child = (data) => { data.hide = () => {}; data.show = () => {}; return data; };
const window = {hidden: false, hide: () => { this.hidden = true; }, show: () => { this.hidden = false; },
    text: child, input: (data) => { return field; },
    button: (data) => { data = child(data); data.on = (event, cb) => { actions :+ cb; }; return data; },
    on: (event, cb) => { keys = cb; }, listen: (target, event, cb) => { listeners[event] = cb; }};
dialog.init({ui: {root: {window: (data) => { test.assert(data.zindex == 0.9); return window; }}}, game: {event: (name, data) => { commands :+ {name: name, data: data}; }}});
let expected = 'prepared';
const data = {available: true, expected_state: 'prepared', credits: 50, reserved: 10, stored: 2, cost: 10, quote: 38, can_full: true};
const base = {id: 701, get_smac_hurry_preview: (amount) => { return {expected_state: expected, paid: amount, minerals_after: 2 + amount / 5}; }};
dialog.open(base, data);
test.assert(dialog.active && dialog.valid_amount && field.value == '38');
test.assert(handlers.change({value: '5'}) == true && dialog.valid_amount && dialog.amount == 5);
test.assert(handlers.input({}) == true && #sizeof(commands) == 1 && !dialog.active);
test.assert(commands[0].name == 'hurry_smac_production' && commands[0].data.base == base && commands[0].data.expected_state == 'prepared' && !commands[0].data.full && commands[0].data.amount == 5);
dialog.submit(false); test.assert(#sizeof(commands) == 1);
for (bad of ['', '-1', '39', '1x', '1.0', ' 1', '01']) {
    dialog.open(base, data); test.assert(handlers.change({value: bad}) == true && !dialog.valid_amount);
    dialog.submit(false); test.assert(#sizeof(commands) == 1);
}
dialog.open(base, data); handlers.change({value: '0'}); dialog.submit(false);
test.assert(#sizeof(commands) == 2 && commands[1].data.amount == 0);
dialog.open(base, data); actions[0](); test.assert(#sizeof(commands) == 3 && commands[2].data.full && commands[2].data.amount == 0);
dialog.open(base, data); actions[2](); test.assert(!dialog.active && #sizeof(commands) == 3);
dialog.open(base, data); keys({modifiers: {}, code: 'ESCAPE'}); test.assert(!dialog.active && #sizeof(commands) == 3);
dialog.open(base, data); dialog.on_base_update(base); test.assert(dialog.active);
expected = 'changed'; handlers.change({value: '5'}); test.assert(!dialog.active && #sizeof(commands) == 3);
expected = 'prepared'; dialog.open(base, data); listeners.research_choice({}); test.assert(!dialog.active);
dialog.open(base, {available: false, expected_state: 'paid', reason: 'Partial payment already used'});
dialog.submit(true); dialog.submit(false); test.assert(dialog.active && #sizeof(commands) == 3 && dialog.info.text == 'Partial payment already used');

dialog.hide_view(); test.assert(focused); dialog.hide_view(); test.assert(focused);
dialog.open(base, data); test.assert(dialog.active && focused); dialog.hide_view();
