const outcome = #include('../default/ui/parts/game/outcome/outcome');
let eliminated = false;
let ending = null;
const prompt = {};
let visible = false;
let resets = 0;
let closed = 0;
let click = null;
let key = null;
const handlers = {};
const game = {
	get_smac_eliminated: () => { return eliminated; },
	get_smac_ending: () => { return ending; },
	on: (name, callback) => { handlers[name] = callback; },
};
const window = {
	text: (properties) => { return prompt; },
	button: (properties) => { return {on: (name, callback) => { click = callback; }}; },
	on: (name, callback) => { key = callback; },
	listen: (object, name, callback) => { object.on(name, callback); },
	show: () => { visible = true; },
	hide: () => { visible = false; },
};
const p = {
	game: game,
	root: {window: (properties) => { return window; }},
	glsmac: {reset: () => { resets++; }},
	modules: {menu: {close_all: () => { closed++; }}},
};
outcome.init(p);
test.assert(!visible && resets == 0 && closed == 0);
eliminated = true;
handlers.turn({});
test.assert(visible && closed == 1 && resets == 0);
click({});
key({code: 'ENTER', modifiers: {}});
test.assert(resets == 2);
// Loading an already eliminated faction shows the outcome without replaying loss.
visible = false;
outcome.init(p);
test.assert(visible && resets == 2);
eliminated = false;
handlers.turn({});
test.assert(!visible && resets == 2);
// A foreign human loss carries a global request independently of local status.
ending = {local: false, faction_name: 'The University'};
handlers.smac_ending({});
test.assert(visible && prompt.text == 'The University has been eliminated.' && !eliminated);
visible = false;
outcome.init(p);
test.assert(visible && prompt.text == 'The University has been eliminated.' && resets == 2);
