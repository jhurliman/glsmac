const view = #include('../default/ui/parts/game/popup/base_screen/top_buttons');
const node = (properties) => {
    const result = properties; result.children = []; result.handlers = {};
    result.on = (event, callback) => { result.handlers[event] = callback; };
    result.listen = (object, event, callback) => {};
    result.hide = () => {}; result.show = () => {};
    const child = (props) => { const value = node(props); result.children :+ value; return value; };
    result.panel = child; result.button = child; result.window = child;
    if (!#is_defined(result.text)) { result.text = child; }
    return result;
};
let calls = [];
const game = {event: (name, args) => { calls :+ {name: name, args: args}; }};
const base = {has_smac_resource_context: true, get_smac_governor_settings: () => { return {expected_state: 'native-token', controls: 524287}; }};
const root = node({});view.init({body: root, ui: {root: root}, game: game});view.set_base(base);view.open();
test.assert(view.active && #sizeof(view.options) == 19 && #sizeof(view.rows) == 19);
for (selected of view.options) { test.assert(selected); }
view.submit();test.assert(#sizeof(calls) == 0 && view.active);
test.assert(view.rows[0].handlers.click({}));view.submit();
test.assert(#sizeof(calls) == 1 && calls[0].name == 'apply_smac_governor' && calls[0].args.controls == 524286 && calls[0].args.expected_state == 'native-token' && !view.active);
view.open();test.assert(view.window.handlers.keydown({modifiers: {}, code: 'ESCAPE'}));test.assert(!view.active && #sizeof(calls) == 1);
view.open();view.set_base(base);test.assert(!view.active);
