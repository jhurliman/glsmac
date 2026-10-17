/*
	normal game start
 */

return (game) => {

	// variables
	const players = game.get_players();
	const conventional_start = game.get_ruleset() == 'smac' && game.get_player().is_single_player;
	const players_count = #sizeof(players);
	const tm = game.get_tm();
	const map_width = tm.get_map_width();
	const map_height = tm.get_map_height();
	// Provisional GLSMAC placement: ordinary land with an exit, and enough
	// separation that another starting HQ/Scout cannot block that first step.
	// The current foreign-sight producer requires clear interior centers.
	// Terrain is preserved; this is not original site-balancing fidelity.
	const minimum_base_distance = conventional_start ? 4 : 2;
	const optimal_base_distance = #max((map_width + map_height) / 2 / players_count, minimum_base_distance);
	let tiles_with_bases = [];

	// functions
	const get_good_starting_base_location = (is_naval_faction) => {
		// find a good tile for starting base
		// very simple for now, TODO: improve/optimize

		// to prevent infinite loop on tiny maps with lots of players
		const max_tries = 1000;
		let tries = 0;

		// will gradually decrease min base distance when struggling to find suitable tile
		let min_base_distance = optimal_base_distance;
		const decrease_min_base_distance_every = max_tries / players_count;

		let next_min_base_distance_decrease_at = decrease_min_base_distance_every;
		while (tries < max_tries) {
			const x = game.random.get_int(0, map_width - 1);
			const y = game.random.get_int(0, map_height - 1);
			if (x % 2 != y % 2) {
				// wrong oddity, try again
				continue;
			}
			const tile = tm.get_tile(x, y);
			let supported_site = tile.is_water == is_naval_faction;
			if (conventional_start) {
				supported_site = false;
				if (tile.is_clear_land && x >= 2 && x + 2 < map_width && y >= 2 && y + 2 < map_height) {
					for (neighbor of tile.get_surrounding_tiles()) {
						if (neighbor != tile && neighbor.is_ordinary_conventional_land) {
							supported_site = true;
							break;
						}
					}
				}
			}
			if (supported_site) { // naval factions start in water, others on land
				// check that no existing bases are too close
				let is_ok = true;
				for (other of tiles_with_bases) {
					if (tm.get_distance(tile, other) < min_base_distance) {
						is_ok = false; // to close
						break;
					}
				}
				if (is_ok) {
					return tile;
				} else {
					if (tries >= next_min_base_distance_decrease_at) {
						// reduce base distance to increase chance of finding tile
						next_min_base_distance_decrease_at = tries + decrease_min_base_distance_every;
						if (min_base_distance > minimum_base_distance) {
							min_base_distance--;
						}
					}
				}
			}
			tries++;
		}
		throw Error('Failed to find good tile for base (try again, or try larger map or fewer players)');
	};

	// initialize each player in game
	for (player of players) {

		const faction = player.get_faction();

		const tile = get_good_starting_base_location(faction.is_naval);

		tiles_with_bases :+tile;

		if (conventional_start) {
			// Native producers resolve actual owner/design and presentation after
			// definition events, still inside the authenticated turn-zero Start.
			game.event('spawn_starting_smac_base', {owner: player, tile: tile});
			game.event('spawn_starting_conventional_unit', {owner: player, tile: tile});
			continue;
		}

		game.event('spawn_base', {owner: player, tile: tile, facilities: ['HEADQUARTERS']});

		// Existing SMAX/network placeholder path.
		let type = 'MindWorms';
		if (faction.is_naval) {
			type = 'SeaLurk';
		}
		game.event('spawn_unit', {
			owner: player,
			tile: tile,
			type: type,
			health: 1.0,
			morale: 1,
		});

	}

};
