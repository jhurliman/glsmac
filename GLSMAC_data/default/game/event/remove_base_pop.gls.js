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
		const old_pop_id = pop.id;
		const old_pop_variant = pop.variant;
		const old_worked_tile = pop.get_worked_tile();

		// reset nutrients
		const old_nutrients = base.get_accumulated_nutrients();
		e.game.get('f_base_reset_nutrients')(e.game, base);

		// remove population
		base.destroy_pop(pop);

		return {
			base: base,
			old_pop_type: old_pop_type,
			old_pop_id: old_pop_id,
			old_pop_variant: old_pop_variant,
			old_nutrients: old_nutrients,
			old_worked_tile: old_worked_tile,
		};
	},

	rollback: (e) => {
		const base = e.data.base;
		base.restore_pop({
			id: e.applied.old_pop_id,
			type: e.applied.old_pop_type,
			variant: e.applied.old_pop_variant,
			worked_tile: e.applied.old_worked_tile,
		});
		base.set_accumulated_nutrients(e.applied.old_nutrients);
	},

};
