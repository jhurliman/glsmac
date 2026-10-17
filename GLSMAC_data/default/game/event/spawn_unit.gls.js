return {

	validate: (e) => {
		if (e.caller != 0) {
			return 'Only master is allowed to spawn units';
		}
	},

	apply: (e) => {
		if (#is_defined(e.data.starting_conventional) && e.data.starting_conventional) {
			if (e.game.get_ruleset() != 'smac' || !e.game.get_player().is_single_player) {
				throw Error('Conventional startup requires local SMAC');
			}
			return {unit: e.game.um.spawn_starting_conventional_unit({
				def: e.data.type, owner: e.data.owner, tile: e.data.tile, render_morale_mode: 0,
			})};
		}
		const unit = e.game.um.spawn_unit({
			def: e.data.type,
			owner: e.data.owner,
			tile: e.data.tile,
			morale: e.data.morale,
			health: e.data.health,
		});

		return {
			unit: unit,
		};
	},

	rollback: (e) => {
		e.game.um.despawn_unit(e.applied.unit);
	},

};
