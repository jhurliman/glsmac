const dialog = #include('../default/ui/parts/game/popup/base_screen/queue_editor');
let commands = []; let listeners = {}; let keys = undefined;
const node = (data) => {
    data.children = []; data.hidden = false;
    data.on = (event, callback) => { if (event == 'click') { data.click = callback; } else { keys = callback; } return data; };
    data.listen = (target, event, callback) => { listeners[event] = callback; };
    data.clear = () => { data.children = []; };
    data.hide = () => { data.hidden = true; }; data.show = () => { data.hidden = false; };
    const child = (properties) => { const value = node(properties); data.children :+ value; return value; };
    data.panel = child; data.listview = child; data.button = child; data.scrollview = child;
    if (!#is_defined(data.text)) { data.text = child; }
    return data;
};
const window = node({});
dialog.init({ui: {root: {window: (data) => { test.assert(data.zindex == 0.9); return window; }}}, game: {event: (name, data) => { commands :+ {name: name, data: data}; }}});
let expected = 'original';
const base = {id: 701, get_smac_specialist_selection_state: () => { return expected; }};
const data = {expected_state: 'original', limit: 9, items: [{existing: 0, name: 'Retained'}, {existing: 1, name: 'Former'}], choices: [{source_id: 2, name: 'Scout'}]};
dialog.open(base, data);
test.assert(dialog.active && !window.hidden && dialog.caption.text == '2 / 9 pending (port storage limit)');
test.assert(dialog.choices.children[0].click() == true && #sizeof(dialog.entries) == 3 && dialog.entries[2].source_id == 2);
let up = undefined;
for (child of dialog.rows.children) { if (child.text == 'Up' && !#is_defined(up)) { up = child.click; } }
test.assert(up() == true && dialog.entries[0].existing == 1 && dialog.entries[1].existing == 0);
let remove = undefined;
for (child of dialog.rows.children) { if (child.text == 'Remove' && !#is_defined(remove)) { remove = child.click; } }
test.assert(remove() == true && #sizeof(dialog.entries) == 2 && dialog.entries[0].existing == 0);
dialog.submit(); dialog.submit();
test.assert(#sizeof(commands) == 1 && commands[0].name == 'edit_smac_queue' && commands[0].data.base == base
    && commands[0].data.expected_state == 'original' && commands[0].data.entries == [{existing: 0}, {source_id: 2}] && !dialog.active);
dialog.open(base, data);
for (let i = 0; i < 12; i++) { dialog.choices.children[0].click(); }
test.assert(#sizeof(dialog.entries) == 9);
keys({modifiers: {}, code: 'ESCAPE'}); test.assert(!dialog.active && #sizeof(commands) == 1);
dialog.open(base, data); dialog.on_base_update(base); test.assert(dialog.active);
expected = 'changed'; dialog.on_base_update(base); test.assert(!dialog.active && #sizeof(commands) == 1);
expected = 'original'; dialog.open(base, data); listeners.research_choice({}); test.assert(!dialog.active);
const screen = #include('../default/ui/parts/game/popup/base_screen');
let opened = false;
base.get_smac_queue_editor = () => { return data; };
screen.base = base; screen.is_open = true; screen.production_selection = null;
screen.sections = {specialist_selection: {hide_view: () => {}}, rename: {hide_view: () => {}}, queue_editor: {
    open: (target, selection) => { test.assert(target == base && selection == data); opened = true; }}};
screen.open_queue_editor(); test.assert(opened);
