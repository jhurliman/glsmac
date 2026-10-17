const factions = #include('../default/factions');
const smac = factions.get_roster('smac');
const smax = factions.get_roster('smax');
test.assert(#sizeof(smac) == 7);
test.assert(#sizeof(smax) == 14);
for (let idx = 0; idx < 7; idx++) { test.assert(smac[idx] == smax[idx]); }
const pirates = smax[8];
test.assert(pirates[0] == 'Pirates');
// The production configure loop binds roster entries directly via for-of.
// Indexed local assignment dereferences entries and can mask ArrayRefs here.
let faction_count = 0;
for (faction of smac) {
	test.assert(#typeof(faction[0]) == 'String');
	faction_count++;
}
test.assert(faction_count == 7);
let rejected = false;
try { factions.get_roster('unselected'); } catch { Error: (e) => { rejected = true; }, }
test.assert(rejected, 'unselected game silently chose a faction roster');

// Use the actual menu step and exercise both buttons before initialization.
const choose = #include('../default/ui/parts/mainmenu/steps/select_ruleset');
// Parse the production entry points as well as the standalone chooser.
const main_menu = #include('../default/ui/parts/mainmenu/steps/main');
#include('../default/ui/parts/mainmenu/mainmenu');
#include('../default/ui/parts/mainmenu/steps/multiplayer_role');
#include('../default/ui/parts/mainmenu/steps/multiplayer_lobby/game_settings');
#include('../default/game/game');
for (let button = 0; button < 2; button++) {
	let calls = [];
	let popup = null;
	const i = {
		glsmac: {
			deinit: () => { calls :+ 'deinit'; },
			select_ruleset: (id) => { calls :+ id; },
		},
		popup: {
			show: (data) => { popup = data; },
			dismiss: () => { calls :+ 'dismiss'; },
		},
	};
	choose(i, () => { calls :+ 'next'; });
	test.assert(calls == [], 'showing chooser initialized gameplay before a choice');
	const selected_button = popup.buttons[button];
	test.assert(selected_button.onclick());
	test.assert(calls == ['deinit', button == 0 ? 'smac' : 'smax', 'dismiss', 'next']);
}

// Both single-player entry points must defer initialization until selection.
for (let entry_index = 0; entry_index < 2; entry_index++) {
	let entries = [];
	let next = null;
	let size_selected = null;
	let initialized = false;
	let continued = false;
	const i = {
		settings: {local: {game_mode: 'multi', network_role: 'client'}},
		glsmac: {
			game: {get_ruleset: () => { return 'smac'; }},
			deinit: () => {},
			init: () => { initialized = true; },
			start_game: () => { continued = initialized; },
		},
		sliding: {show: (data) => { entries = data.entries; }},
		steps: {
			select_ruleset: (unused, callback) => { next = callback; },
			select_mapsize: (unused, callback) => { size_selected = callback; },
			start_game: (unused) => { continued = initialized; },
		},
		randomize_map: () => {},
	};
	main_menu(i);
	const entry = entries[entry_index];
	entry[1]();
	test.assert(!initialized && !continued);
	test.assert(i.settings.local.game_mode == 'single' && i.settings.local.network_role == 'none');
	next();
	if (entry_index == 1) {
		test.assert(initialized && !continued, 'SMAC quickstart bypassed explicit size selection');
		size_selected();
	}
	test.assert(initialized && continued);
}
