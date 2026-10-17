return {

	validate: (e) => {
		if (e.caller != 0) {
			return 'Only master is allowed to spawn bases';
		}
	},

	apply: (e) => {
		const options = {
			// name: e.data.name,
			facilities: e.data.facilities,
		};
		if (#is_defined(e.data.smac_starting_resource_context)) {
			options.smac_starting_resource_context = e.data.smac_starting_resource_context;
		}
		const base = e.game.bm.spawn_base(e.data.owner, e.data.tile, options);

		return {
			base: base,
		};
	},

	rollback: (e) => {
		e.game.bm.despawn_base(e.applied.base);
	},

};
