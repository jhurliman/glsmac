const view = #include('../default/ui/parts/game/popup/base_screen/energy_rates');
const node = (properties) => {
    const result = properties; result.children = []; result.handlers = {}; result.visible = true;
    result.on = (event, callback) => { result.handlers[event] = callback; };
    result.listen = (object, event, callback) => { object.on(event, callback); };
    result.hide = () => { result.visible = false; }; result.show = () => { result.visible = true; };
    const child = (props) => { const value = node(props); result.children :+ value; return value; };
    result.button = child; result.window = child;
    if (!#is_defined(result.text)) { result.text = child; }
    return result;
};
let calls = []; let listeners = {};
const game = {event: (name, args) => { calls :+ {name: name, args: args}; }, on: (name, callback) => { listeners[name] = callback; }};
const base = {get_smac_energy_rate_preview: () => { return {expected_state: 'native-owner-state'}; }};
const root = node({});view.init({ui: {root: root}, game: game});view.set_base(base);view.open();
test.assert(!view.active && !view.window.visible && #sizeof(calls) == 1 && calls[0].name == 'open_smac_energy_rates' && calls[0].args.expected_state == 'native-owner-state');
let data = {economy: 5, labs: 5, psych: 0, bases: [{economy: 7, labs: 4, psych: 0}, {economy: 2, labs: 1, psych: 1}]};
listeners.smac_energy_rates_menu({request_id: 1, closed: false, selection: data});
test.assert(view.active && view.window.visible && view.rows[0].value.text == '50%' && view.rows[2].value.text == '0%');
test.assert(view.rows[2].lock.handlers.click({}) && #sizeof(calls) == 1);
let pluses = [];
for (child of view.window.children) { if (#typeof(child.text) == 'String' && child.text == '+') { pluses :+ child; } }
test.assert(#sizeof(pluses) == 3 && pluses[2].handlers.click({}) && pluses[2].handlers.click({}));
test.assert(#sizeof(calls) == 3 && calls[1].name == 'adjust_smac_energy_rates' && calls[1].args == {request_id: 1, button: 34, locks: 2} && calls[2].args == calls[1].args);
data.labs = 3; data.psych = 2;
listeners.smac_energy_rates_menu({request_id: 1, closed: false, selection: data});
view.set_base(base);
test.assert(view.active && view.rows[2].value.text == '20%' && view.locked[1]);
view.hide_view();test.assert(view.active && view.closing && view.window.visible && #sizeof(calls) == 4 && calls[3].args == {request_id: 1, cancel: true});
view.hide_view();test.assert(#sizeof(calls) == 4);
listeners.smac_energy_rates_menu({request_id: 99, closed: true});test.assert(view.active);
listeners.smac_energy_rates_menu({request_id: 1, closed: true});test.assert(!view.active && !view.window.visible);
view.open();listeners.smac_energy_rates_menu({request_id: 2, closed: false, selection: data});
test.assert(view.active && !view.locked[1]);
test.assert(view.window.handlers.keydown({modifiers: {}, code: 'ENTER'}));
test.assert(calls[5].name == 'close_smac_energy_rates' && calls[5].args == {request_id: 2, cancel: false});
listeners.smac_energy_rates_menu({request_id: 2, closed: true});
listeners.smac_energy_rates_menu({request_id: 1, closed: false, selection: data});test.assert(!view.active);
// A parent dismissed before the opening acknowledgment must cancel the native
// session when that acknowledgment arrives, without stranding its modal fence.
view.open();view.hide_view();listeners.smac_energy_rates_menu({request_id: 3, closed: false, selection: data});
test.assert(view.closing && calls[7].args == {request_id: 3, cancel: true});
listeners.smac_energy_rates_menu({request_id: 3, closed: true});
view.open();listeners.smac_energy_rates_menu({request_id: 4, closed: false, selection: data});
test.assert(view.window.handlers.keydown({modifiers: {}, code: 'ESCAPE'}) && calls[9].args == {request_id: 4, cancel: true});
