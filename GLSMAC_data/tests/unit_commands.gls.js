// UI-only smoke test. Native Former/Colony tests supply the real wrappers and
// command admission; these doubles make callback/layout syntax testable alone.
const initialize = #include('_unit_commands');
let handlers = {};
let map_handlers = {};
let events = [];
let offered = [1, 2, 3, 4, 5];
let order = 0;
let progress = 0;
let kind = 'former';
let held = false;
const unit = {id: 71, is_conventional: true, query_conventional_commands: () => {
	return {kind: kind, actions: offered, work_order: order, work_progress: progress, held: held};
}};
const game = {
	on: (name, callback) => { handlers[name] = callback; },
	get_map: () => { return {on: (name, callback) => {map_handlers[name] = callback;}}; },
	event: (name, data) => { events :+{name: name, data: data}; },
};
const check = initialize(game);
handlers.unit_select({unit: unit});
check('check', 'Farm [F]|Road [R]|Forest [O]|Solar [S]|Mine [M]', 'Former — Order: None  |  Stored work: 0');
check('remember', 'Farm [F]', '');
check('click', 'Farm [F]', '');
test.assert(#sizeof(events) == 1 && events[0].name == 'work_unit' && events[0].data.unit.id == 71 && events[0].data.action == 1);
order = 1;
progress = 2;
offered = [1, 2, 3, 4, 5, 0];
map_handlers.unit_preview({unit: unit});
check('check', 'Farm [F]|Road [R]|Forest [O]|Solar [S]|Mine [M]|Cancel [C]', 'Former — Order: Farm  |  Stored work: 2');
check('click', 'Cancel [C]', '');
test.assert(#sizeof(events) == 2 && events[1].name == 'cancel_unit_work' && events[1].data.unit.id == 71);
const other = {id: 72, is_conventional: true, query_conventional_commands: unit.query_conventional_commands};
handlers.unit_select({unit: other});
check('remembered', '', '');
test.assert(#sizeof(events) == 2);
handlers.unit_select({unit: unit});
offered = [];
check('remembered', '', '');
test.assert(#sizeof(events) == 2);
map_handlers.unit_preview({unit: unit});
check('check', '', 'Former — Order: Farm  |  Stored work: 2');
kind = 'colony';
offered = [6];
map_handlers.unit_preview({unit: unit});
check('check', 'Found Base [B]', 'Colony Pod — Found a Base on an available site');
check('click', 'Found Base [B]', '');
test.assert(#sizeof(events) == 3 && events[2].name == 'found_base' && events[2].data.unit.id == 71);
handlers.smac_governor_foundation({unit: unit, expected_state: 'pod-state'});
check('click', 'Cancel', '');
test.assert(#sizeof(events) == 3);
handlers.smac_governor_foundation({unit: unit, expected_state: 'pod-state'});
check('click', 'Use default Governor settings', '');
test.assert(#sizeof(events) == 4 && events[3].name == 'found_base_with_default_governor' && events[3].data.expected_state == 'pod-state');
handlers.unit_despawn({unit_id: 71});
check('hidden', '', '');
handlers.unit_select({unit: {id: 99, is_conventional: false}});
check('hidden', '', '');

held = true; kind = 'unit'; offered = [7];
handlers.unit_select({unit: unit});
check('check', 'Activate unit', 'Holding position — stored work: 2');
check('click', 'Activate unit', '');
test.assert(#sizeof(events) == 5 && events[4].name == 'unit_activate' && events[4].data.unit.id == 71);
