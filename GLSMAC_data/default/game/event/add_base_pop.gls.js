return {

	validate: (e) => {
		if (e.caller != 0) {
			return 'Only master is allowed to add base pop';
		}
	},

	apply: (e) => {

		const base = e.data.base;

		// reset nutrients
		const old_nutrients = base.get_accumulated_nutrients();
		e.game.get('f_base_reset_nutrients')(e.game, base);

		// spawn population
		const pop = base.create_pop({
			type: e.data.type,
			worked_tile: e.data.worked_tile,
		});

		return {
			base: base,
			pop: pop,
			old_nutrients: old_nutrients,
		};
	},

	rollback: (e) => {
		e.data.base.destroy_pop(e.applied.pop);
		e.data.base.set_accumulated_nutrients(e.applied.old_nutrients);
	},

};
