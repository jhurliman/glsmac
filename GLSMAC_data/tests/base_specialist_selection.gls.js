// Actual optional specialist modal and native event payloads.
const dialog = #include('../default/ui/parts/game/popup/base_screen/specialist_selection');
let footer = [];
let choices = [];
let events = [];
let key = undefined;
let listeners = {};
const button = () => { return {on: (event, callback) => { this.click = callback; }}; };
const list = {clear: () => { choices = []; }, button: (data) => { const b = button(); choices :+ b; return b; }};
const window = {
	hidden: false,
	hide: () => { this.hidden = true; }, show: () => { this.hidden = false; },
	text: (data) => { return data; }, listview: (data) => { return list; },
	button: (data) => { const b = button(); footer :+ b; return b; },
	on: (event, callback) => { key = callback; },
	listen: (target, event, callback) => { listeners[event] = callback; },
};
dialog.init({ui: {root: {window: (data) => { test.assert(data.zindex == 0.9); return window; }}},
	game: {event: (name, data) => { events :+ {name: name, data: data}; }}});
let state = 'after-prelude';
const base = {id: 701, name: 'Gaia', get_smac_specialist_selection_state: () => { return state; }};
const data = {pop_id: 19, request_id: 7, expected_state: 'after-prelude', current_type: -1, minimum_size: 5,
	size_explanation_requested: true, creation_limited: false, choices: [{type: -1, name: 'Worker'}, {type: 1, name: 'Doctor'}]};
dialog.open(base, data);
test.assert(dialog.active && !window.hidden && dialog.info.text == 'Some types require Base size 5.');
choices[1].click(); choices[0].click();
test.assert(dialog.selected == -1);
choices[1].click(); footer[0].click(); footer[0].click();
test.assert(!dialog.active && window.hidden && #sizeof(events) == 1);
test.assert(events[0].name == 'select_smac_specialist' && events[0].data.base == base && events[0].data.type == 1
	&& events[0].data.pop_id == 19 && events[0].data.request_id == 7 && events[0].data.expected_state == 'after-prelude');
dialog.open(base, data);
dialog.on_base_update(base);
test.assert(dialog.active);
state = 'changed'; dialog.on_base_update(base);
test.assert(!dialog.active && #sizeof(events) == 1);
state = 'after-prelude'; dialog.open(base, data);
listeners.research_choice({});
test.assert(!dialog.active);
dialog.open(base, data);
key({modifiers: {}, code: 'ESCAPE'});
test.assert(!dialog.active && #sizeof(events) == 2 && events[1].name == 'cancel_smac_specialist');
dialog.cancel(); test.assert(#sizeof(events) == 2);
const screen = #include('../default/ui/parts/game/popup/base_screen');
screen.is_open = true; screen.base_id = 701; screen.production_selection = null;
screen.p = {game: {event: (name, data) => { events :+ {name: name, data: data}; }}};
screen.open_specialist_selection(base, 19);
test.assert(events[2].name == 'open_smac_specialist' && events[2].data.expected_state == 'after-prelude');
state = undefined; screen.open_specialist_selection(base, 19);
test.assert(#sizeof(events) == 3);

// Actual Pop surfaces retain distinct Base/population identities after refresh.
const citizens = #include('../default/ui/parts/game/popup/base_screen/bottom_bar/middle_area');
let surfaces = [];
let hits = [];
const node = () => { return {
    panel: (data) => { return node(); }, text: (data) => { return data; }, button: (data) => { return node(); },
    on: (event, callback) => { this.click = callback; }, show: () => {}, hide: () => {}, clear: () => {},
    surface: (data) => { const result = node(); surfaces :+ result; return result; },
}; };
citizens.init({body: node(), ui: {class: (name) => { return {extend: (name) => { return this; }, set: (data) => {}}; }},
    game: {get_bm: () => { return {get_pop_renders: (owner) => { return {worker: ['sprite']}; }}; }, get_pending_smac_colony_screen: () => { return null; }},
    utils: {open_specialist_selection: (target, id) => { hits :+ {base: target, pop_id: id}; }}});
const target = {get_smac_base_navigation: () => { return undefined; }};
citizens.set({name: 'First', owner: {}, base: target, pops: [
    {id: 19, variant: 0, get_type: () => { return 'worker'; }},
    {id: 20, variant: 0, get_type: () => { return 'worker'; }}]});
surfaces[1].click({}); surfaces[0].click({});
test.assert(hits == [{base: target, pop_id: 20}, {base: target, pop_id: 19}]);
