const prefs = #include('../default/ui/parts/game/preferences/preferences');
let windows = []; let key = null; let messages = []; let writes = 0; let fail = false; let measure_fail = false; let live = {}; let defaults = {};
let pages = [];
for (ids of [[7,8,9,10,11,12,13,14],[15,16,17,18,19,28,20,21,23,24,25,26],
    [43,44,45,46,47,48,49,50,51,52,54,55,56],[29,30,31,32,33,34,42,35,36,37,38,39,40,41],
    [64,65,66,71,67,68,69,22],[72,73,78,74,75,76,77,79],[57,58,59,60]]) {
    let rows = []; for (id of ids) { rows :+ {id: id, label: 'option'}; const name = #to_string(id); live[name] = false; defaults[name] = true; }
    pages :+ {title: 'page', rows: rows};
}
const words = ['Preferences','Advanced','Warnings','Automation','Audio/Visual','Map','Gamma','Defaults','OK','Cancel','More','Back'];
const node = (data) => {
    test.assert(!#is_defined(data._active) && !#is_defined(data._hover));
    data.handlers = {}; data.children = []; data.hidden = false; data.removed = false;
    data.hide = () => { data.hidden = true; }; data.show = () => { data.hidden = false; }; data.remove = () => { data.removed = true; };
    data.is_visible = () => { return !data.hidden; };
    data.on = (event, callback) => { data.handlers[event] = callback; };
    data.measure = () => { if (measure_fail) { throw Error('Missing caption font'); } return {width: 18}; };
    data.smacpreferencestext = () => { return pages; }; data.smacpreferencescaptions = () => { return words; };
    const child = (properties) => { const result = node(properties); data.children :+ result; return result; };
    data.surface = child; data.panel = child; data.button = child;
    if (!#is_defined(data.text)) { data.text = child; }
    return data;
};
const editor = {active: false, opening: false};
const popup = {popup: null, popup_defs: {base_screen: {sections: {workshop: editor}}}};
popup.popup_defs.base_screen.sections.workshop = editor;
let closed = 0; let ruleset = 'smac';
const p = {ui: {class: (name) => { return {set: (style) => { test.assert(#is_defined(style._active)); }}; },
    root: {window: (props) => { const window = node(props); windows :+ window; return window; }},
    get_width: () => { return 1024; }, get_height: () => { return 768; }},
    root: {on: (event, callback) => { key = callback; }}, game: {get_ruleset: () => { return ruleset; },
        get_settings: () => { return {global: {difficulty_level: 'Librarian'}}; }},
    modules: {popup: popup, menu: {close_all: () => { closed++; }}, first_base: {pending: null}, research: {pending: null}, colony: {pending: null}},
    glsmac: {config: {get_smac_preferences: (laptop,difficulty) => { test.assert(!laptop && difficulty == 3); return {values: live, audio: {a: true}}; },
        get_smac_preference_defaults: (laptop) => { return {values: defaults, audio: {a: true}}; },
        set_smac_preferences: (values,audio) => { if (fail) { throw Error('Cannot write'); } live = values; writes++; }}},
    process_message: (text) => { messages :+ text; }};
p.modules.popup = popup; prefs.init(p);
test.assert(!key({code: 'P', modifiers: {}}) && !key({code: 'P', modifiers: {ctrl: true, shift: true}}));
test.assert(key({code: 'P', modifiers: {ctrl: true}}) && prefs.active && closed == 1 && #sizeof(prefs.rows) == 67);
test.assert(prefs.window.left == 162 && prefs.window.top == 150 && prefs.groups[0].hidden == false && prefs.groups[1].hidden);
test.assert(prefs.rows[1].control.element.handlers.click({button: 'left'}) && prefs.model.values['8'] && !live['8']);
prefs.model.select(1); prefs.refresh(); prefs.model.select(0); prefs.refresh();
test.assert(prefs.model.values['8'] && prefs.rows[1].control.checked);
test.assert(prefs.key({code: 'ESCAPE', modifiers: {}}) && !prefs.active && !live['8'] && writes == 0);
prefs.open(0); test.assert(!prefs.rows[1].control.checked);
prefs.rows[1].control.element.handlers.click({button: 'left'}); prefs.finish(true);
test.assert(live['8'] && writes == 1 && !prefs.active && prefs.window.hidden);
prefs.open(2); test.assert(!prefs.more.hidden); prefs.more.handlers.click({button: 'left'}); test.assert(prefs.model.page == 6 && prefs.more.text == 'Back');
prefs.more.handlers.click({button: 'left'}); test.assert(prefs.model.page == 2); prefs.finish(false);
prefs.open(0); prefs.model.restore(); prefs.refresh(); prefs.rows[1].control.element.handlers.click({button: 'left'});
test.assert(!prefs.model.values['8'] && !prefs.rows[1].control.checked && defaults['8']);
fail = true; prefs.finish(true); test.assert(prefs.active && prefs.model.active && !prefs.window.hidden && live['8'] && #sizeof(messages) == 1);
fail = false; prefs.finish(false);
for (name of ['first_base','research','colony']) { p.modules[name].pending = {}; test.assert(!prefs.open(0)); p.modules[name].pending = null; }
editor.active = true; test.assert(!prefs.open(0)); editor.active = false;
popup.popup = {}; test.assert(!prefs.open(0)); popup.popup = null;
ruleset = 'smax'; test.assert(!prefs.open(0)); ruleset = 'smac';
const routes = #include('../default/ui/parts/game/menu/left_menu_game'); let entries = []; let opened = (0 - 1);
routes.init({create: (rows) => { entries = rows; }, open_preferences: (page) => { opened = page; }});
test.assert(#sizeof(entries) == 9); entries[1].open(); test.assert(opened == 2);
prefs.window.remove(); prefs.window = undefined; measure_fail = true;
test.assert(prefs.open(0) && !prefs.active && !#is_defined(prefs.window));
measure_fail = false; test.assert(prefs.open(0) && prefs.active); prefs.finish(false);
