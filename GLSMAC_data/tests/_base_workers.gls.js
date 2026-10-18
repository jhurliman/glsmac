// Real base callbacks and native resource validation, with deterministic tile yields.
const initialize_bases = #include('../default/game/bases');
return (bm, tm, get_resources) => {
	let start = null;
	const helpers = {};
	const game = {
		get_ruleset: () => { return 'smac'; },
        is_campaign_restore: () => { return false; },
		configure_smac_base_presentations: (roles) => {}, // definitions are substituted
		get_bm: () => { return bm; },
		get_tm: () => { return tm; },
		event: (name, data) => {},
		on: (name, callback) => { if (name == 'start') { start = callback; } },
		set: (name, value) => { helpers[name] = value; },
		get: (name) => { return helpers[name]; },
	};
	tm.on('get_tile_resources', #is_defined(get_resources) ? get_resources : (e) => {
		return { NUTRIENTS: e.tile.x + 1, MINERALS: 1, ENERGY: 0 };
	});
	initialize_bases(game);
	start({});
	return game;
};
