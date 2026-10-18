// Port migration question: real callbacks, no inferred answer or gameplay mutation.
const initialize = #include('./_smac_colony_choice');
let pending = {kind: 'former_owner', request_id: 31, base_id: 701, base_name: 'Test Base', owner_slot: 0,
    choices: [{id: 0, name: 'Native life'}, {id: 1, name: 'Gaians'}, {id: 2, name: 'Spartans'}]};
const handlers = {};
let events = [];
let screen = null;
const game = {
    get_pending_smac_colony: () => { return pending; },
    get_pending_smac_colony_screen: () => { return screen; },
    on: (name, callback) => { handlers[name] = callback; },
    event: (name, args) => { events :+{name: name, args: args}; },
};
const ui = initialize(game, () => {});
test.assert(ui.state().visible && !ui.state().submitted && #sizeof(events) == 0);
test.assert(ui.confirm_former() && #sizeof(events) == 0);
ui.choose(2); test.assert(#sizeof(events) == 0);
test.assert(ui.cancel_former() && events == [{name: 'choose_former_faction', args: {request_id: 31, former_owner: -1}}]);
test.assert(ui.state().visible && !ui.state().submitted);
ui.confirm_former(); test.assert(#sizeof(events) == 1);
ui.choose(0); ui.confirm_former();
test.assert(events[1] == {name: 'choose_former_faction', args: {request_id: 31, former_owner: 0}} && ui.state().submitted);
// An unpublished preparation error reannounces the same stable question.
handlers.colony_choice({}); test.assert(ui.state().visible && !ui.state().submitted);
ui.confirm_former(); test.assert(#sizeof(events) == 2);
ui.choose(1); ui.confirm_former();
test.assert(events[2] == {name: 'choose_former_faction', args: {request_id: 31, former_owner: 1}});
pending = null; handlers.colony_choice({});
test.assert(!ui.state().visible && !ui.state().submitted);
