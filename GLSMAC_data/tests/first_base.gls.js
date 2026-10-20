const module = #include('../default/ui/parts/game/first_base/first_base');
let pending = {request_id: 1, name: 'Landing', max_bytes: 23};
let visible = false;
let input = null;
let event = null;
let keydown = null;
let buttons = {};
let sent = [];
let input_alive = false;
let focused = false;
let window = null;
window = {
    surface: (style) => {test.assert(style.width == 8 && style.height == 529); return {};},
    panel: (style) => {return window;},
    hide: () => {visible = false;}, show: () => {visible = true;},
    text: (style) => {return {text: style.text};},
    input: (style) => {
        test.assert(style.max_bytes == 23);
        test.assert(!input_alive); input_alive = true;
        input = {value: style.value, on: (name, callback) => {event = callback;},
            focus: () => {focused = true;}, remove: () => {test.assert(input_alive); input_alive = false; focused = false;}}; return input;
    },
    button: (style) => {return {on: (name, callback) => {buttons[style.text] = callback;}};},
    on: (name, callback) => {keydown = callback;},
    listen: (game, name, callback) => {test.assert(name == 'first_base_name'); this.notify = callback;},
};
module.init({
    game: {get_pending_first_base_name: () => {return pending;}, finish_first_base_name: (id, name) => {sent :+[id, name];}},
    root: {window: (style) => {
        test.assert(style.left == 238 && style.top == 255 && style.width == 580 && style.height == 300 && #sizeof(style.smac_frame) == 4 && style.blocker == 'transparent');return window;
    }},
    modules: {menu: {close_all: () => {}}},
    glsmac: {},
});
test.assert(visible && focused && input.value == 'Landing');
input.value = 'Edited';module.refresh();test.assert(input.value == 'Edited');
test.assert(event({}) == true && #sizeof(sent) == 1 && (sent[0])[1] == 'Edited');
test.assert(buttons.OK() == true && #sizeof(sent) == 1, 'Duplicate result was submitted');
pending = null;module.refresh();test.assert(!visible && !input_alive);
pending = {request_id: 2, name: 'Next', max_bytes: 23};module.refresh();
test.assert(!#is_defined(buttons.Cancel), 'FIRSTBASE must have only its installed OK control');
input.value = 'Escape retains this';test.assert(keydown({modifiers: {}, code: 'ESCAPE'}) == true && (sent[1])[1] == 'Escape retains this');
pending = {request_id: 3, name: 'Third', max_bytes: 23};module.refresh();
input.value = '';test.assert(keydown({modifiers: {}, code: 'ESCAPE'}) == true && (sent[2])[1] == '');
pending = null;module.refresh();module.refresh();test.assert(!input_alive && !visible);
