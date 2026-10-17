return {

	validate: (e) => {
		if (e.caller != 0) {
			return 'Only master is allowed to remove base pop';
		}
	},

	apply: (e) => {

		const base = e.data.base;
		const pop = e.data.pop;

		const old_pop_type = pop.get_type();
		const old_worked_tile = pop.get_worked_tile();

		// reset nutrients
		const old_nutrients = base.get_accumulated_nutrients();
		e.game.get('f_base_reset_nutrients')(e.game, base);

		// remove population
		base.destroy_pop(pop);

		return {
			base: base,
			old_pop_type: old_pop_type,
			old_nutrients: old_nutrients,
			old_worked_tile: old_worked_tile,
		};
	},

	rollback: (e) => {
		const base = e.data.base;
		const pop = base.create_pop({
			type: e.applied.old_pop_type,
			worked_tile: e.applied.old_worked_tile,
		});
		base.set_accumulated_nutrients(e.applied.old_nutrients);
	},

};
