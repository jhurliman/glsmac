// Actual screen wiring and energy/account sections; only assets and unrelated
// sections are doubled. This does not establish native rendered pixel parity.
const screen = #include('../default/ui/parts/game/popup/base_screen');
const allocation = #include('../default/ui/parts/game/popup/base_screen/energy');
const account = #include('../default/ui/parts/game/popup/base_screen/game_state');
allocation.labels = {};
for (type of ['economy', 'labs', 'psych']) {
	allocation.labels[type] = {percentage: {}, value: {}, bonus: {}, total: {}};
}
account.info_year = {};
account.info_energy = {};
account.info_ecodamage = {};
let balance = null;
let reads = 0;
let eco_damage = 137;
let nutrient_view = {stored: 13, threshold: 20};
let displayed_nutrients = undefined;
let response = {credits: 137, raw_intake: 15, waste: 3, net: 12, maintenance: 2,
	rates: {economy: 5, labs: 5, psych: 0}, allocated: {economy: 7, labs: 8, psych: 2},
	final: {economy: 7, labs: 12, psych: 2}};
const base = {
	id: 71, has_smac_resource_context: true,
	get_owner: () => { return {get_faction: () => { return {is_progenitor: false}; }}; },
	get_smac_resource_calculation: () => { return {
		eco_damage: eco_damage, nutrients: {intake: 6, demand: 7}, minerals: {intake: 18, demand: 2}, raw_intake: {ENERGY: 15},
	}; },
	get_smac_energy_calculation: () => { reads++; return response; },
	get_accumulated_nutrients: () => { return 13; },
	get_smac_nutrient_presentation: () => { return nutrient_view; },
	get_facilities: () => { return {}; },
};
const sink = {set: (data) => {}, set_stored: (value) => {}, set_commerce: (value) => {}};
screen.sections = {hurry: {on_base_update: (base) => {}}, queue_editor: {on_base_update: (base) => {}}, buttons: {show_default: () => {}}, rename: {on_base_update: (base) => {}}, specialist_selection: {on_base_update: (base) => {}}, nutrients: {set_stored: (data) => { displayed_nutrients = data; }}, economy: sink, game_state: account, facilities: sink,
	resources: {set: (data) => {balance = data;}}, energy: allocation, middle_area: sink, bottom_bar: sink};
screen.p = {game: {get_year: () => {return 2101;}}};
screen.production_selection = null;
screen.set({base: base});
test.assert(reads == 1 && account.info_year.text == '2101' && account.info_energy.text == '137');
test.assert(displayed_nutrients == nutrient_view);
test.assert(account.info_ecodamage.text == 'Eco-Damage: 137');
test.assert(balance.energy == {profit: 15, loss: 3});
test.assert(balance.minerals == {profit: 18, loss: 2});
test.assert(allocation.labels.labs.percentage.text == ' 50%' && allocation.labels.psych.percentage.text == '  0%');
test.assert(allocation.labels.labs.value.text == '  8' && allocation.labels.labs.bonus.text == '  4' && allocation.labels.labs.total.text == ' 12');
test.assert(response.credits == 137 && response.allocated.labs == 8 && response.final.labs == 12);

// A reopened/refreshed screen consumes the new authority, including negative
// adjustments, without retaining an old treasury or inventing a positive bonus.
eco_damage = 0;
response = {credits: 91, raw_intake: 10, waste: 2, net: 8, maintenance: 0,
	rates: {economy: 0, labs: 10, psych: 0}, allocated: {economy: 0, labs: 8, psych: 0},
	final: {economy: 0, labs: 6, psych: 0}};
screen.set({base: base});
test.assert(reads == 2 && account.info_energy.text == '91' && balance.energy.loss == 2);
test.assert(account.info_ecodamage.text == 'Eco-Damage: 0');
test.assert(allocation.labels.labs.percentage.text == '100%' && allocation.labels.labs.bonus.text == ' -2' && allocation.labels.labs.total.text == '  6');

// Partial legacy canonical Base records retain explicit unavailable values.
eco_damage = null;
response = undefined;
nutrient_view = undefined;
screen.set({base: base});
test.assert(reads == 3 && account.info_energy.text == 'N/A' && !#is_defined(balance.energy.loss));
test.assert(account.info_ecodamage.text == 'Eco-Damage: N/A');
for (type of ['economy', 'labs', 'psych']) {
	test.assert(allocation.labels[type].percentage.text == 'N/A' && allocation.labels[type].total.text == 'N/A');
}

test.assert(displayed_nutrients == {stored: 13});
