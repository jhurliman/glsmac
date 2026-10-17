return {

	validate: (e) => {
		if (e.caller != e.data.base.get_owner().id) {
			return 'Only base owner can control base pops';
		}
		const work_pop = e.data.tile.get_working_pop();
		if (!#is_defined(work_pop)) {
			return 'Tile is not being worked';
		}
		if (work_pop.get_base() != e.data.base) {
			return 'Population does not belong to this base';
		}
	},

	apply: (e) => {
		const pop = e.data.tile.get_working_pop();
		const old_type = pop.get_type();
		e.game.get('f_base_pop_unwork_tile')(e.data.base, pop, 'DOCTOR');
		return {
			pop: pop,
			old_type: old_type,
		};
	},

	rollback: (e) => {
		e.data.base.set_pop_assignment(e.applied.pop, e.applied.old_type, e.data.tile);
	},

};
