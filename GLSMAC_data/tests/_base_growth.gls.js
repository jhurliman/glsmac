// Keep real game callbacks, helper storage and queued events. Only asset-bearing
// definitions are supplied by the native fixture instead of these init events.
const initialize_bases = #include('../default/game/bases');
return (game) => {
	initialize_bases({
		get_ruleset: game.get_ruleset,
		// This adapter suppresses definitions; native fixtures supply their roles.
		configure_smac_base_presentations: (roles) => {},
		get_bm: game.get_bm,
		get_tm: game.get_tm,
		on: game.on,
		set: game.set,
		get: game.get,
		event: (name, data) => {
			if (name != 'define_base_pop' && name != 'define_base_facility') {
				game.event(name, data);
			}
		},
	});
};
