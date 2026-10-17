const select_rules = #include('../default/ui/parts/mainmenu/steps/select_rules');
const select_faction = #include('../default/ui/parts/mainmenu/steps/select_faction');
const popup_factory = #include('../default/ui/parts/mainmenu/popup');
const catalog = #include('../default/factions');

// Actual rules/faction steps, popup and back stack. Only native startup and DOM
// construction are replaced; each callback must pass its configured stable ID.
for (ruleset of ['smac', 'smax']) {
	let factions = [];
	for (entry of catalog.get_roster(ruleset)) {
		const id = #uppercase(entry[0]);
		const label = entry[0];
		factions :+{id: id, name: label};
	}
	let shown = null;
	let title = '';
	let clicks = {};
	let keydown = null;
	let choices = null;
	let starts = 0;
	let selected = null;
	let resets = 0;
	let manager = null;
	const sliding = {
		show: (data) => { shown = data; manager.push('sliding', data); },
		select: (index) => {},
	};
	manager = #include('../default/ui/parts/mainmenu/manager')('sliding', sliding);
	const glsmac = {
		config: {},
		game: {get_fm: () => { return {list: () => { return factions; }}; }},
		start_game: (id) => { starts++; selected = id; },
		reset: () => { resets++; },
		exit: () => { throw TestError('Faction cancellation exited'); },
	};
	const popup = popup_factory({
		glsmac: glsmac,
		root: {window: (data) => {
			title = data.title;
			clicks = {};
			return {
				remove: () => {},
				on: (event, callback) => { keydown = callback; },
				panel: (style) => { return {
					choicelist: (data) => { choices = {items: data.items, value: (data.items[0])[0]}; return choices; },
					button: (style) => { return {on: (event, callback) => { clicks[style.text] = callback; }}; },
				};
				},
			};
		}},
	});
	const i = {glsmac: glsmac, popup: popup, sliding: sliding, steps: {select_faction: select_faction}};
	for (let rules_choice = 0; rules_choice < 2; rules_choice++) {
		select_rules(i);
		const rules = shown;
		((rules.entries[rules_choice])[1])();
		test.assert(title == 'Select Faction' && starts == 0);
		test.assert(#sizeof(choices.items) == (ruleset == 'smac' ? 8 : 15));
		if (rules_choice == 0) { clicks.Cancel(); }
		else { keydown({code: 'ESCAPE', modifiers: []}); }
		test.assert(shown.title == 'Game rules' && starts == 0 && resets == 0);
		popup.hide();
	}
	for (let choice = 0; choice <= #sizeof(factions); choice++) {
		select_rules(i);
		((shown.entries[0])[1])();
		choices.value = (choices.items[choice])[0];
		if (choice != 0) {
			const faction = factions[choice - 1];
			test.assert((choices.items[choice])[0] == faction.id && (choices.items[choice])[1] == faction.name);
		}
		clicks.OK();
		test.assert(starts == choice + 1);
		if (choice == 0) { test.assert(!#is_defined(selected), 'Random must retain the native no-argument path'); }
		else { test.assert(selected == (factions[choice - 1]).id, 'Menu lost the chosen faction ID'); }
		popup.hide();
	}
}
