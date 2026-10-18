const detailed = #include('../default/ui/parts/game/popup/base_screen/middle_area/support');
const compact = #include('../default/ui/parts/game/popup/base_screen/bottom_bar/support');
let lines = [];
const items = {clear: () => {lines = [];}, text: (p) => {lines :+ p.text;}};
detailed.items = items;
compact.items = items;
const units = [
	{id: 91, name: 'Scout', status: 'Free', minerals: 0, x: 6, y: 6},
	{id: 33, name: 'Former', status: 'Paid', minerals: 1, x: 8, y: 6},
	{id: 12, name: 'Future exemption', status: 'Exempt', minerals: 0, x: 8, y: 8},
];
const presentation = {current: {minerals: 1, eligible: 2, free_allowance: 1, paid: 1, units: units}, retained: {minerals: 0}};
detailed.set(presentation);
test.assert(lines == ['Current preview: 1 minerals', 'Eligible: 2  Free allowance: 1  Paid: 1', 'Scout #91: Free at 6,6', 'Former #33: Paid (1) at 8,6', 'Future exemption #12: Exempt at 8,8', 'Last resource pass: 0 minerals']);
compact.set(presentation);
test.assert(lines == ['Current preview: 1 minerals', 'Scout #91: Free', 'Former #33: Paid (1)', 'Future exemption #12: Exempt', 'Last resource pass: 0 minerals']);
compact.set({current: {minerals: 0, units: []}, retained: undefined});
test.assert(lines == ['Current preview: 0 minerals', 'No units homed here', 'Last resource pass unavailable']);
detailed.set(undefined);
test.assert(lines == ['Support unavailable']);

compact.set(undefined);
test.assert(lines == ['Support unavailable']);
