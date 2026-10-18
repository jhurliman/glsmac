// Compose the actual list's deferred Base opening/close callback with the
// actual outcome module. Only DOM, popup presentation and Game reads are stubs.
const objects = #include('../default/ui/parts/game/bottom_bar/objects_list');
const outcome = #include('../default/ui/parts/game/outcome/outcome');
const handlers = {};
let ending = null;
let visible = false;
let popup_shown = false;
let close_screen = null;
let selections = 0;
let resets = 0;
let menu_closes = 0;
const prompt = {};
const base = {id: 702};
const game = {
	get_smac_ending: () => { return ending; },
	get_smac_eliminated: () => { return false; },
	select_tile: (tile) => { selections++; },
	on: (name, callback) => {
		if (!#is_defined(handlers[name])) { handlers[name] = []; }
		handlers[name] :+ callback;
	},
};
const frame = {};
frame.panel = (properties) => { return frame; };
frame.listen = (object, name, callback) => { object.on(name, callback); };
frame.on = (name, callback) => {};
const window = {
	text: (properties) => { return prompt; },
	button: (properties) => { return {on: (name, callback) => {}}; },
	listen: (object, name, callback) => { object.on(name, callback); },
	on: (name, callback) => {},
	show: () => { visible = true; },
	hide: () => { visible = false; },
};
const p = {
	game: game, map: {on: (name, callback) => {}}, el: frame,
	ui: {class: (name) => { return {set: (properties) => {}}; }},
	root: {window: (properties) => { return window; }},
	glsmac: {reset: () => { resets++; }},
	modules: {
		outcome: outcome,
		menu: {close_all: () => { menu_closes++; }},
		popup: {
			set: (name, data) => { test.assert(name == 'base_screen' && data.base == base); },
			show: (name, callback) => { popup_shown = true; close_screen = callback; },
		},
	},
};
outcome.init(p);
objects.init(p);
objects.bases['702'] = {item: {active: false}};
test.assert(!visible && !popup_shown && resets == 0);
handlers.base_select[0]({base: base});
#async(10, () => {
	test.assert(popup_shown && !visible && selections == 0 && menu_closes == 0);
	// No turn or ending event is emitted while Base Control is open. Capture
	// has committed a foreign human loss while local focus remains the captor.
	ending = {kind: 'human_defeat', local: false, faction_name: 'The University'};
	popup_shown = false;
	close_screen({});
	test.assert(visible && !popup_shown && selections == 1 && menu_closes == 1 && resets == 0);
	test.assert(prompt.text == 'The University has been eliminated.');
	return false;
});
