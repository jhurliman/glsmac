const popup_factory = #include('../default/ui/parts/mainmenu/popup');
const multiplayer_type = #include('../default/ui/parts/mainmenu/steps/multiplayer_type');
const multiplayer_role = #include('../default/ui/parts/mainmenu/steps/multiplayer_role');
const multiplayer_host = #include('../default/ui/parts/mainmenu/steps/multiplayer_host');
const select_ruleset = #include('../default/ui/parts/mainmenu/steps/select_ruleset');

// Execute the real popup, history manager, and multiplayer steps. Only DOM
// creation and backend initialization are replaced with synchronous test surfaces.
for (let choice = 0; choice < 2; choice++) {
	let title = '';
	let clicks = {};
	let keydown = null;
	let resets = 0;
	let exits = 0;
	let ancestor_destructions = 0;
	let initialized = false;
	let selected = null;
	const settings = {
		local: {
			network_type: 'none',
			network_role: 'none',
			account: {game_name: 'Test game', player_name: 'Test player'},
		},
		global: {},
	};
	const glsmac = {
		config: {},
		game: {get_settings: () => { return settings; }},
		deinit: () => { initialized = false; },
		select_ruleset: (id) => { selected = id; },
		init: () => { initialized = true; },
		reset: () => { resets++; },
		exit: () => { exits++; },
	};
	const m = {
		glsmac: glsmac,
		root: {
			window: (data) => {
				title = data.title;
				clicks = {};
				return {
					remove: () => {},
					on: (event, callback) => { keydown = callback; },
					panel: (style) => {
						return {
							text: (data) => {},
							input: (data) => { return {value: data.value}; },
							choicelist: (data) => {
								const first = data.items[0];
								return {value: first[0]};
							},
							button: (style) => {
								return {on: (event, callback) => { clicks[style.text] = callback; }};
							},
						};
					},
				};
			},
		},
	};
	const popup = popup_factory(m);
	const i = {
		popup: popup,
		glsmac: glsmac,
		settings: settings,
		steps: {
			multiplayer_role: multiplayer_role,
			multiplayer_host: multiplayer_host,
			select_ruleset: select_ruleset,
		},
	};
	popup.show({title: 'Previous menu', destructor: () => { ancestor_destructions++; }});
	multiplayer_type(i);
	clicks.OK();
	test.assert(title == 'Host or Join Session');
	clicks.OK();
	test.assert(title == 'Game Ruleset' && !initialized);
	// ESC from the chooser still navigates back normally, without selecting.
	keydown({code: 'ESCAPE', modifiers: []});
	test.assert(title == 'Host or Join Session' && selected == null && !initialized);
	clicks.OK();
	const button_name = choice == 0 ? 'Alpha Centauri' : 'Alien Crossfire';
	const select = clicks[button_name];
	select();
	test.assert(title == 'Host Multiplayer Game' && initialized);
	test.assert(selected == (choice == 0 ? 'smac' : 'smax'));
	test.assert(ancestor_destructions == 0, 'ruleset choice destroyed earlier menu history');
	// Cover both the host form's Cancel button and its production ESC handler.
	if (choice == 0) { clicks.Cancel(); }
	else { keydown({code: 'ESCAPE', modifiers: []}); }
	test.assert(title == 'Host or Join Session', 'host cancellation did not return to role selection');
	test.assert(resets == 0 && exits == 0, 'host cancellation unexpectedly reset or exited');
	clicks.Cancel();
	test.assert(title == 'Multiplayer Setup', 'multiplayer service history was lost');
	clicks.Cancel();
	test.assert(title == 'Previous menu', 'ancestor menu history was lost');
	popup.hide();
	test.assert(ancestor_destructions == 1, 'closing navigation did not run ancestor cleanup once');
}
