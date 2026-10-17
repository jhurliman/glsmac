const initialize = #include('./_smac_colony_choice');
let pending = {request_id: 11, base_id: 701, base_name: 'Gaia\'s Landing', owner_slot: 0};
const handlers = {};
let events = [];
let resets = 0;
const game = {
	get_pending_smac_colony: () => { return pending; },
	on: (name, callback) => { handlers[name] = callback; },
	event: (name, args) => { events :+{name: name, args: args}; },
};
const ui = initialize(game, () => { resets++; });
test.assert(ui.state().visible && !ui.state().submitted && #sizeof(events) == 0);
ui.key('ESCAPE', {}); ui.key('ENTER', {});
test.assert(#sizeof(events) == 0 && ui.state().visible);
const stale = ui.state().buttons[2].handlers.click;
ui.choose(0);
test.assert(events == [{name: 'choose_colony', args: {request_id: 11, choice: 'delay'}}]);
ui.choose(1); ui.choose(2); handlers.colony_choice({});
test.assert(#sizeof(events) == 1 && ui.state().submitted && ui.state().visible);

pending = {request_id: 12, base_id: 702, base_name: 'The Hive', owner_slot: 1};
handlers.colony_choice({}); stale({});
test.assert(!ui.state().submitted && #sizeof(events) == 1);
ui.choose(1);
test.assert(events[1] == {name: 'choose_colony', args: {request_id: 12, choice: 'screen'}});
pending = null; handlers.colony_choice({});
test.assert(!ui.state().visible && ui.state().pending == null && !ui.state().submitted);

pending = {request_id: 13, base_id: 701, base_name: 'Gaia\'s Landing', owner_slot: 0};
handlers.colony_choice({}); ui.choose(2);
test.assert(events[2] == {name: 'choose_colony', args: {request_id: 13, choice: 'accept'}});
ui.reset(); ui.key('Q', {ctrl: true, shift: true});
test.assert(resets == 2 && #sizeof(events) == 3);
