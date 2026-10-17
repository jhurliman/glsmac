const unready_players = (players) => {
	for (player of players) {
		player.set_ready(false);
	}
};

return {

	validate: (e) => {
		if (e.game.is_started()) {
			return 'Game has already started';
		}
		if (e.caller != 0) {
			return 'Only master is allowed to change game settings';
		}
		if (e.game.get_ruleset() == 'smac') {
			let accepted_size = false;
			let raw_size = false;
			for (c of e.data.changes) {
				if (c[0] == 'planet_size') {
					if (accepted_size) { return 'Only one world-size choice is allowed per change'; }
					accepted_size = true;
					if (c[1] != '0' && c[1] != '1' && c[1] != '2' && c[1] != '3' && c[1] != '4') {
						return 'Select a SMAC world-size preset';
					}
				}
				if (c[0] == 'size_x' || c[0] == 'size_y') { raw_size = true; }
			}
			if (accepted_size && raw_size) { return 'A world-size choice cannot be mixed with raw dimensions'; }
			if (raw_size) { return 'Use an accepted world-size choice instead of raw dimensions'; }
		}
	},

	apply: (e) => {
		let old_settings = [];
		const global = e.game.get_settings().global;
		let settings = global.map;
		let changes = [];
		for (c of e.data.changes) {
			if (c[0] == 'planet_size') {
				if (global.ruleset == 'smac') {
					old_settings :+['world_size', global.get_world_size()];
					global.set_world_size(#to_int(c[1]));
				} else {
					const xy = #split(c[1], 'x');
					changes :+['size_x', #to_int(xy[0])];
					changes :+['size_y', #to_int(xy[1])];
				}
			} else {
				changes :+c;
			}
		}
		for (c of changes) {
			const oldv = settings[c[0]];
			if (#is_defined(oldv)) { // TODO: support all fields
				old_settings :+[c[0], oldv];
				settings[c[0]] = c[1]; // TODO: ro/rw permissions on settings in all places
			}
		}

		unready_players(e.game.get_players());

		e.game.trigger('game_settings', {
			settings: e.data.changes,
		});
		return {
			old_settings: old_settings,
		};
	},

	rollback: (e) => {
		const global = e.game.get_settings().global;
		let settings = global.map;
		let restored_settings = [];
		for (c of e.applied.old_settings) {
			if (c[0] == 'world_size') {
				const previous = c[1];
				global.set_world_size(previous[0], previous[1], previous[2]);
				restored_settings :+['planet_size', #to_string(previous[0])];
			} else { settings[c[0]] = c[1]; restored_settings :+c; }
		}

		unready_players(e.game.get_players());

		e.game.trigger('game_settings', {
			settings: restored_settings
		});
	},

};
