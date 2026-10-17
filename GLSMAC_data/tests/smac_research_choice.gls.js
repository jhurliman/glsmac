const initialize = #include('./_smac_research_choice');
let pending = null;
const handlers = {};
let events = [];
let resets = 0;
const game = {
	get_pending_smac_research: () => { return pending; },
	on: (name, callback) => { handlers[name] = callback; },
	event: (name, args) => { events :+{name: name, args: args}; },
};
const request = {request_id: 41, owner_slot: 3, owner_name: 'University', bonus: false,
	choices: [{id: 8, name: 'Centauri Ecology', category: 0}, {id: 3, name: 'Information Networks', category: 1}]};
// Already pending at UI setup; construction and Enter never select by default.
pending = request;
const ui = initialize(game, () => { resets++; });
test.assert(ui.state().pending == request && ui.state().selected == null && #sizeof(events) == 0);
test.assert(ui.state().labels[0].text == 'Centauri Ecology (Explore)' && ui.state().labels[1].text == 'Information Networks (Discover)');
ui.key('ENTER', {});
ui.key('ESCAPE', {});
test.assert(#sizeof(events) == 0 && ui.state().pending == request && resets == 0);
ui.choose(1);
ui.confirm();
test.assert(events == [{name: 'choose_research', args: {request_id: 41, technology_id: 3}}]);
test.assert(ui.state().submitted && ui.state().pending == request);
ui.confirm(); ui.key('ENTER', {}); ui.choose(0);
test.assert(#sizeof(events) == 1 && ui.state().selected == 3);
handlers.research_choice({}); // repeated same request must not enable resubmission
test.assert(ui.state().submitted);

// A later bonus request replaces the pool and selection; stale callbacks cannot
// choose an entry from the retired request.
const stale = ui.state().labels[0].handlers.click;
pending = {request_id: 42, owner_slot: 0, owner_name: 'Gaians', bonus: true,
	choices: [{id: 4, name: 'Industrial Base', category: 2}, {id: 5, name: 'Applied Physics', category: 3}]};
handlers.research_choice({});
test.assert(ui.state().selected == null && !ui.state().submitted && ui.state().prompt == 'Gaians: choose a bonus discovery');
stale({}); test.assert(ui.state().selected == null);
ui.choose(0); ui.key('ENTER', {});
test.assert(events[1] == {name: 'choose_research', args: {request_id: 42, technology_id: 4}});
pending = null; handlers.research_choice({});
test.assert(ui.state().pending == null && !ui.state().submitted);
ui.confirm(); test.assert(#sizeof(events) == 2);

// An ordinary event can open after setup; returning to the main menu uses only
// the real reset route, never a fake choice/cancel command.
pending = request; handlers.research_choice({});
ui.reset(); test.assert(resets == 1 && #sizeof(events) == 2);
ui.key('Q', {ctrl: true, shift: true});
test.assert(resets == 2 && #sizeof(events) == 2);
